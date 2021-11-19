/*--------------------------------------------------------------*
  Copyright (C) 2006-2020 OpenSim Ltd.

  This file is distributed WITHOUT ANY WARRANTY. See the file
  'License' for details on this and other legal matters.
*--------------------------------------------------------------*/

package org.omnetpp.scave;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

import org.omnetpp.common.util.StringUtils;
import org.omnetpp.common.util.XmlUtils;
import org.omnetpp.scave.charttemplates.ChartTemplateRegistry;
import org.omnetpp.scave.model.Analysis;
import org.omnetpp.scave.model.Chart;
import org.omnetpp.scave.model.Chart.ChartType;
import org.omnetpp.scave.model.ChartTemplate;
import org.omnetpp.scave.model.InputFile;
import org.omnetpp.scave.model.Property;
import org.w3c.dom.Element;
import org.w3c.dom.Node;
import org.w3c.dom.NodeList;

/**
 * Loads an Analysis model (object hierarchy) from an analysis (.anf) file.
 *
 * Uses hand-written logic to extract the model from the XML DOM of
 * the analysis file, to understand not only the latest file format,
 * but also some slightly different, development variants of it.
 *
 * For loading legacy analysis files (saved by OMNeT++ versions older than 6.0),
 * see LegacyAnalysisLoader.
 */
public class AnalysisLoader {

    /**
     * Helper method to extract the text content of an element - excluding
     * any indentation around a CDATA section in it, if present. If there
     * is no CDATA section in the element, return the entire text contents.
     */
    private static String extractCdataOrTextContent(Element scriptElement) {
        int n = scriptElement.getChildNodes().getLength();
        for (int k = 0; k < n; ++k) {
            Node childNode = scriptElement.getChildNodes().item(k);
            // to make sure the indentation before the CDATA section is not included (it can mess up the script)
            if (childNode.getNodeType() == Node.CDATA_SECTION_NODE) {
                return childNode.getTextContent();
            }
        }
        // just return the entire text content of the <script> tag
        return scriptElement.getTextContent();
    }

    /**
     * Loads an Analysis model from the given XML DOM root node, resolving
     * chart tempalate IDs to references in the given registry.
     */
    public static Analysis loadNewAnalysis(Node rootNode, ChartTemplateRegistry chartTemplateRegistry) {

        if (!rootNode.getNodeName().equals("analysis"))
            throw new RuntimeException("Invalid top level node: " + rootNode.getNodeName());

        Analysis analysis = new Analysis();

        Node versionNode = rootNode.getAttributes().getNamedItem("version");

        if (versionNode == null)
            throw new RuntimeException("Unspecified analysis file version");

        if (!"2".equals(versionNode.getNodeValue()))
            throw new RuntimeException("invalid analysis file version: " + versionNode.getNodeValue());

        NodeList topLevelNodes = rootNode.getChildNodes();

        for (int i = 0; i < topLevelNodes.getLength(); ++i) {
            Node node = topLevelNodes.item(i);

            if ("inputs".equals(node.getNodeName())) {
                NodeList inputNodes = node.getChildNodes();
                for (int j = 0; j < inputNodes.getLength(); ++j) {
                    Node inputNode = inputNodes.item(j);
                    if ("input".equals(inputNode.getNodeName())) {
                        Node patternNode = inputNode.getAttributes().getNamedItem("pattern");
                        InputFile input = new InputFile(patternNode.getNodeValue());

                        analysis.getInputs().addInput(input);
                    } else if ("#text".equals(inputNode.getNodeName())) {
                        if (!inputNode.getNodeValue().trim().isEmpty())
                            throw new RuntimeException("Unexpected text content: " + inputNode.getNodeValue());
                    } else {
                        throw new RuntimeException("Invalid child node: " + inputNode.getNodeName());
                    }
                }
            }
            else if ("charts".equals(node.getNodeName())) {
                NodeList chartNodes = node.getChildNodes();

                for (int j = 0; j < chartNodes.getLength(); ++j) {
                    Node chartNode = chartNodes.item(j);

                    if ("chart".equals(chartNode.getNodeName())) {

                        Node typeNode = chartNode.getAttributes().getNamedItem("type");

                        String chartType = typeNode.getNodeValue();

                        Chart chart = new Chart();

                        try {
                            ChartType type = ChartType.valueOf(chartType);
                            chart.setType(type);
                        }
                        catch (IllegalArgumentException e) {
                            switch (chartType) {
                                case "MatplotlibChart": chart.setType(ChartType.MATPLOTLIB); break;
                                case "BarChart": chart.setType(ChartType.BAR); break;
                                case "LineChart": chart.setType(ChartType.LINE); break;
                                case "ScatterChart": chart.setType(ChartType.LINE); break; // for backward compatibility
                                case "HistogramChart": chart.setType(ChartType.HISTOGRAM); break;
                                default: throw new RuntimeException("Unknown chart type: " + chartType);
                            }
                        }

                        Node idNode = chartNode.getAttributes().getNamedItem("id");
                        if (idNode != null)
                            chart.setId(Integer.parseInt(idNode.getNodeValue()));
                        chart.setName(chartNode.getAttributes().getNamedItem("name").getNodeValue());

                        Node templateNode = chartNode.getAttributes().getNamedItem("template");
                        if (templateNode != null)
                            chart.setTemplateID(templateNode.getNodeValue());

                        Node iconPathNode = chartNode.getAttributes().getNamedItem("icon");
                        if (iconPathNode != null)
                            chart.setIconPath(iconPathNode.getNodeValue());
                        else {
                            ChartTemplate template = chartTemplateRegistry.findTemplateByID(chart.getTemplateID());
                            if (template != null)
                                chart.setIconPath(template.getIconPath());
                        }

                        Node scriptAttrNode = chartNode.getAttributes().getNamedItem("script");
                        Element scriptElement = XmlUtils.getFirstChildElementWithTag(chartNode, "script");

                        if (scriptAttrNode != null)
                            chart.setScript(scriptAttrNode.getNodeValue());
                        else {
                            if (scriptElement != null)
                                chart.setScript(extractCdataOrTextContent(scriptElement));
                            else
                                chart.setScript(StringUtils.stripEnd(chartNode.getTextContent(), " "));
                        }

                        Element formElement = XmlUtils.getFirstChildElementWithTag(chartNode, "form");
                        if (formElement != null) {
                            List<Chart.DialogPage> pages = Arrays.asList(new Chart.DialogPage[] {
                                    new Chart.DialogPage("PROPERTIES", "Properties", formElement.getTextContent()) });
                            chart.setDialogPages(pages);
                        }
                        else {
                            ArrayList<Chart.DialogPage> pages = new ArrayList<>();

                            List<Element> pageNodes = XmlUtils.getChildElementsWithTag(chartNode, "dialogPage");
                            for (Element pageNode : pageNodes) {
                                String id = pageNode.getAttribute("id");
                                String label = pageNode.getAttribute("label");
                                String xswtForm = extractCdataOrTextContent(pageNode);
                                Chart.DialogPage page = new Chart.DialogPage(id, label, xswtForm);
                                pages.add(page);
                            }
                            chart.setDialogPages(pages);
                        }

                        List<Element> propNodes = XmlUtils.getChildElementsWithTag(chartNode, "property");
                        for (Element propNode : propNodes) {
                            String name = propNode.getAttribute("name");
                            String value = propNode.getAttribute("value");
                            Property prop = new Property(name, value);
                            chart.addProperty(prop);
                        }

                        analysis.getRootFolder().add(chart);
                    }
                    else if ("#text".equals(chartNode.getNodeName())) {
                        if (!chartNode.getNodeValue().trim().isEmpty())
                            throw new RuntimeException("Unexpected text content: " + chartNode.getNodeValue());
                    } else {
                        throw new RuntimeException("Invalid child node: " + chartNode.getNodeName());
                    }
                }
            }
            else if ("#text".equals(node.getNodeName())) {
                if (!node.getNodeValue().trim().isEmpty())
                    throw new RuntimeException("Unexpected text content: " + node.getNodeValue());
            }
            else {
                throw new RuntimeException("Invalid child node: " + node.getNodeName());
            }
        }

        return analysis;
    }

}
