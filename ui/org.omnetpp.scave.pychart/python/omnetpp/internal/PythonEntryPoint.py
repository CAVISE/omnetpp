"""
The initial Python module that sets up the Py4J connection between the
IDE (Java), and a python3 process spawned by it (Python).
"""

__copyright__ = "Copyright 2016-2020, OpenSim Ltd"
__license__ = """
  This file is distributed WITHOUT ANY WARRANTY. See the file
  'License' for details on this and other legal matters.
"""

import sys
import os
import pickle as pl

# the print function is replaced so it will flush after each line
import functools
print = functools.partial(print, flush=True)

# the manual conversions are only necessary because of a Py4J bug:
# https://github.com/bartdag/py4j/issues/359
from py4j.java_collections import MapConverter, ListConverter

from omnetpp.internal.TimeAndGuard import TimeAndGuard
from omnetpp.internal import Gateway

from py4j.java_gateway import DEFAULT_PORT
from py4j.clientserver import ClientServer, JavaParameters, PythonParameters

try:
    import matplotlib as mpl
    import matplotlib.pyplot as plt
    import numpy as np
    import scipy as sp
    import pandas as pd
except ImportError as e:
    print("can't import " + e.name)
    sys.exit(1)


class PythonEntryPoint(object):
    """
    An implementation of the Java interface org.omnetpp.scave.pychart.IPythonEntryPoint
    through Py4J. Provides an initial point to set up additional object references between
    Java and Python, and to execute arbitrary Python code from the IDE.
    """

    class Java:
        implements = ["org.omnetpp.scave.pychart.IPythonEntryPoint"]

    def __init__(self):
        """ The execution context of the scripts submitted to execute() """
        self.execContext = {
            "print": print,
            "exit": sys.exit
        }

    def check(self):
        return True

    def setResultsProvider(self, results_provider):
        Gateway.results_provider = results_provider

    def setChartProvider(self, chart_provider):
        Gateway.chart_provider = chart_provider

    def setPlotWidgetProvider(self, widget_provider):
        Gateway.widget_provider = widget_provider

    def setNativeChartPlotter(self, chart_plotter):
        Gateway.chart_plotter = chart_plotter

    # @TimeAndGuard(measureTime=False)
    def execute(self, chartInput):
        # these imports must be lazy, to allow correct module initialization
        from omnetpp.scave import chart, ideplot
        try:
            exec(chartInput, self.execContext)
        except chart.ChartScriptError as e:
            ideplot.set_warning(str(e))
            sys.exit(1)
        except Exception as e:
            import traceback
            traceback.print_exc()
            ideplot.set_warning(str(e))
            sys.exit(1)

    # @TimeAndGuard(measureTime=False)
    def evaluate(self, expression):
        return eval(expression, self.execContext)

    def getRcParams(self):
        return MapConverter().convert({str(k) : str(v) for k,v in mpl.rcParams.items()}, Gateway.gateway._gateway_client)

    def getVectorOps(self):
        from omnetpp.scave import vectorops

        ops = []
        # ctor is: VectorOp(String module, String name, String signature, String docstring, String label, String example)
        for o in vectorops._report_ops():
            try:
                ops.append(Gateway.gateway.jvm.org.omnetpp.scave.editors.VectorOperations.VectorOp(*o))
            except Exception as E:
                print("Exception while processing vector operation:", o[4])

        return ListConverter().convert(ops, Gateway.gateway._gateway_client)

    def setGlobalObjectPickle(self, name, pickle):
        self.execContext[name] = pl.loads(pickle)


def connect_to_IDE():
    java_port = int(sys.argv[1]) if len(sys.argv) > 1 else DEFAULT_PORT
    # print("initiating Python ClientServer, connecting to port " + str(java_port))

    entry_point = PythonEntryPoint()

    gateway = ClientServer(
        java_parameters=JavaParameters(port=java_port, auto_field=True, auto_convert=True, auto_close=True),
        python_parameters=PythonParameters(port=0, daemonize=True, daemonize_connections=True),
        python_server_entry_point=entry_point)

    Gateway.gateway = gateway

    python_port = gateway.get_callback_server().get_listening_port()

    # telling Java which port we listen on
    address = gateway.java_gateway_server.getCallbackClient().getAddress()
    gateway.java_gateway_server.resetCallbackClient(address, python_port)

    # print("Python ClientServer done, listening on port " + str(python_port))


def setup_unbuffered_output():
    # I believe the purpose of the following piece of code is entirely achieved by the "-u" command line argument.
    # But just to be sure, let's leave this in here, I don't think it will cause harm.
    # We're just trying to make the stdout and stderr streams unbuffered, so all output is sure(r) to reach the Console.
    if os.name == "posix":
        import fcntl

        fl = fcntl.fcntl(sys.stdout.fileno(), fcntl.F_GETFL)
        fl |= os.O_SYNC
        fcntl.fcntl(sys.stdout.fileno(), fcntl.F_SETFL, fl)

        fl = fcntl.fcntl(sys.stderr.fileno(), fcntl.F_GETFL)
        fl |= os.O_SYNC
        fcntl.fcntl(sys.stderr.fileno(), fcntl.F_SETFL, fl)
    else:
        import msvcrt
        msvcrt.setmode(sys.stdout.fileno(), os.O_BINARY)
        msvcrt.setmode(sys.stderr.fileno(), os.O_BINARY)


def setup_pandas_display_parameters():
    pd.set_option("display.width", 500)
    pd.set_option("display.max_columns", 50)
    pd.set_option("display.max_colwidth", 50)
    pd.set_option("display.max_rows", 500)


if __name__ == "__main__":

    setup_unbuffered_output()
    setup_pandas_display_parameters()
    connect_to_IDE()

    # block the main Python thread while the host process is running
    # (our stdin is connected to it via a pipe, so stdin will be closed when it exists)
    for line in sys.stdin:
        # We don't actually expect any input, this is just a simple way to wait
        # for the parent process (Java) to die.
        pass

    # print("Python process exiting...")

    Gateway.gateway.close(False, True)
    Gateway.gateway.shutdown_callback_server()
    Gateway.gateway.shutdown()

    sys.exit()
    # it should never come to this, but just to make sure:
    os._exit(1)
