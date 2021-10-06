import math
from omnetpp.scave import results, chart, utils, ideplot

# get chart properties
props = chart.get_properties()
utils.preconfigure_plot(props)

# collect parameters for query
filter_expression = props["filter"]
start_time = float(props["vector_start_time"] or -math.inf)
end_time = float(props["vector_end_time"] or math.inf)

# query vector data into a data frame
try:
    df = results.get_vectors(filter_expression, include_attrs=True, include_itervars=True, start_time=start_time, end_time=end_time)
except ValueError as e:
    ideplot.set_warning("Error while querying results: " + str(e))
    exit(1)

if df.empty:
    ideplot.set_warning("The result filter returned no data.")
    exit(1)

# apply vector operations
df = utils.perform_vector_ops(df, props["vector_operations"])

# plot
utils.plot_vectors(df, props)

utils.postconfigure_plot(props)

utils.export_image_if_needed(props)
utils.export_data_if_needed(df, props)
