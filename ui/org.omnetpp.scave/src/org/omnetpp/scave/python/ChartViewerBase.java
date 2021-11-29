/*--------------------------------------------------------------*
  Copyright (C) 2006-2020 OpenSim Ltd.

  This file is distributed WITHOUT ANY WARRANTY. See the file
  'License' for details on this and other legal matters.
*--------------------------------------------------------------*/

package org.omnetpp.scave.python;

import java.io.File;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import org.eclipse.swt.widgets.Control;
import org.omnetpp.common.Debug;
import org.omnetpp.scave.ScavePlugin;
import org.omnetpp.scave.editors.ChartProvider;
import org.omnetpp.scave.editors.FilterCache;
import org.omnetpp.scave.editors.MemoizationCache;
import org.omnetpp.scave.editors.ResultsProvider;
import org.omnetpp.scave.engine.ResultFileManager;
import org.omnetpp.scave.model.Chart;
import org.omnetpp.scave.pychart.PythonCallerThread.ExceptionHandler;
import org.omnetpp.scave.pychart.PythonOutputMonitoringThread.IOutputListener;
import org.omnetpp.scave.pychart.PythonProcess;
import org.omnetpp.scave.pychart.PythonProcessPool;
import org.omnetpp.scave.python.MatplotlibChartViewer.IStateChangeListener;

public abstract class ChartViewerBase {

    protected Chart chart;
    protected ResultFileManager rfm;

    protected PythonProcess proc = null;
    protected PythonProcessPool processPool;
    protected MemoizationCache memoizationCache;
    protected FilterCache filterCache;

    protected List<IOutputListener> outputListeners = new ArrayList<IOutputListener>();
    protected List<IStateChangeListener> stateChangeListeners = new ArrayList<IStateChangeListener>();

    private ChartProvider chartProvider;

    public ChartViewerBase(Chart chart, PythonProcessPool processPool, ResultFileManager rfm, MemoizationCache memoizationCache, FilterCache filterCache) {
        this.processPool = processPool;
        this.chart = chart;
        this.rfm = rfm;
        this.memoizationCache = memoizationCache;
        this.filterCache = filterCache;
    }

    public Chart getChart() {
        return chart;
    }

    public void killPythonProcess() {
        if (proc != null && proc.isAlive())
            proc.kill();
    }

    public boolean isAlive() {
        return proc != null && proc.isAlive();
    }

    public void addOutputListener(IOutputListener listener) {
        outputListeners.add(listener);
    }

    public void addStateChangeListener(IStateChangeListener listener) {
        stateChangeListeners.add(listener);
    }

    protected void acquireNewProcess() {
        killPythonProcess();

        proc = processPool.getProcess();
        Debug.println("acquired new python process for chartviewerbase: PID " + proc.getProcess().pid());

        for (MatplotlibChartViewer.IStateChangeListener l : stateChangeListeners)
            l.pythonProcessLivenessChanged(proc);

        for (IOutputListener l : outputListeners) {
            proc.outputMonitoringThread.addOutputListener(l);
            proc.errorMonitoringThread.addOutputListener(l);
        }

        proc.getEntryPoint().setResultsProvider(new ResultsProvider(rfm, proc, memoizationCache, filterCache));
        proc.getEntryPoint().setChartProvider(chartProvider = new ChartProvider(chart));

        proc.getProcess().onExit().thenRun(() -> {
            for (MatplotlibChartViewer.IStateChangeListener l : stateChangeListeners)
                l.pythonProcessLivenessChanged(proc);
        });

    }

    protected void changePythonIntoDirectory(File workingDir) {
        proc.getEntryPoint().execute("import os; os.chdir(r\"\"\"" + workingDir.getAbsolutePath() + "\"\"\"); del os;");
        proc.getEntryPoint().execute("import site; site.addsitedir(r\"\"\"" + workingDir.getAbsolutePath() + "\"\"\"); del site;");

        if (PythonProcess.getMatplotlibRcParams() == null) {
            try {
                Map<String, String> rcParams = ((Map<String, String>)proc.getEntryPoint().getRcParams());
                PythonProcess.setMatplotlibParams(rcParams);
            }
            catch (Exception e) {
                ScavePlugin.logError("Could not query rcParams from Python, content assist for it won't be available", e);
                PythonProcess.setMatplotlibParams(new HashMap<String,String>()); // don't try again next time
            }
        }
    }

    public PythonProcess getPythonProcess() {
        return proc;
    }

    public void dispose() {
        killPythonProcess();
    }

    public String getSuggestedChartName() {
        return chartProvider == null ? null : chartProvider.getSuggestedChartName();
    }

    public List<String> getObservedColumnNames() {
        return chartProvider == null ? null : chartProvider.getObservedColumnNames();
    }

    public abstract void runPythonScript(String script, File workingDir, Runnable runAfterDone, ExceptionHandler runAfterError);

    public abstract Control getWidget();

    public abstract void setVisible(boolean visible);

    public abstract void copyImageToClipboard();

    public abstract void saveImage(String fileName);


}
