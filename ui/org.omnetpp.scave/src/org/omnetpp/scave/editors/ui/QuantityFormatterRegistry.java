/*--------------------------------------------------------------*
  Copyright (C) 2006-2022 OpenSim Ltd.

  This file is distributed WITHOUT ANY WARRANTY. See the file
  'License' for details on this and other legal matters.
*--------------------------------------------------------------*/

package org.omnetpp.scave.editors.ui;

import java.util.ArrayList;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import org.omnetpp.common.engine.JavaMatchableObject;
import org.omnetpp.common.engine.MatchExpression;
import org.omnetpp.common.engine.QuantityFormatter;
import org.omnetpp.common.engine.StringVector;
import org.omnetpp.common.engineext.IMatchableObject;

public class QuantityFormatterRegistry
{
    private static QuantityFormatterRegistry instance;

    public static QuantityFormatterRegistry getInstance() {
        if (instance == null)
            instance = new QuantityFormatterRegistry();
        return instance;
    }

    public static class Mapping {
        public String expression;
        public MatchExpression matcher;
        public QuantityFormatter.Options options;
        public String testInput;

        public Mapping(String expression, QuantityFormatter.Options options, String testInput) {
            this.options = options;
            this.testInput = testInput;
            setExpression(expression);
        }

        public void setExpression(String expression) {
            this.expression = expression;
            this.matcher = new MatchExpression(expression, false, false, false);
        }

        public String computeTestOutput() {
            if (testInput == null)
                return null;
            else {
                Pattern pattern = Pattern.compile("([-\\.\\d]+) *([^\\d]+)?");
                Matcher matcher = pattern.matcher(testInput.strip());
                if (matcher.find()) {
                    String valueText = matcher.group(1);
                    String unit = matcher.groupCount() > 1 ? matcher.group(2) : null;
                    QuantityFormatter quantityFormatter = new QuantityFormatter(options);
                    if (valueText.equals(""))
                        return "";
                    else {
                        try {
                            double value = Double.parseDouble(valueText);
                            QuantityFormatter.Output output = quantityFormatter.formatQuantity(value, unit);
                            return output.getText();
                        }
                        catch (RuntimeException e) {
                            // void
                            return null;
                        }
                    }
                }
                else
                    return null;
            }
        }
    }

    private ArrayList<Mapping> mappings = new ArrayList<Mapping>();
    private JavaMatchableObject javaMatchableObject = new JavaMatchableObject();

    public QuantityFormatter getQuantityFormatter(IMatchableObject matchableObject) {
        javaMatchableObject.setJavaObject(matchableObject);
        QuantityFormatter.Options quantityFormatterOptions = null;
        for (int i = 0; i < mappings.size(); i++) {
            Mapping mapping = mappings.get(i);
            if (mapping.matcher.matches(javaMatchableObject)) {
                quantityFormatterOptions = mapping.options;
                break;
            }
        }
        if (quantityFormatterOptions == null)
            quantityFormatterOptions = new QuantityFormatter.Options();
        return new QuantityFormatter(quantityFormatterOptions);
    }

    public ArrayList<Mapping> getMappings() {
        return mappings;
    }
}