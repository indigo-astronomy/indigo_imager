// Copyright (c) 2025 Rumen G.Bogdanovski
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHORS 'AS IS' AND ANY EXPRESS
// OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include <QRegularExpression>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QDebug>
#include "SequenceItemModel.h"
#include "IndigoSequenceParser.h"

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#define QT_SKIP_EMPTY_PARTS Qt::SkipEmptyParts
#else
#define QT_SKIP_EMPTY_PARTS QString::SkipEmptyParts
#endif


/* The Sequencer library never had clear_focus_selection(), the call is
   clear_focuser_selection(). Sequences saved with the old name are still loaded
   under the corrected one. */
static QString resolve_legacy_name(const QString &name) {
	if (name == SC_CLEAR_FOCUS_SELECTION_LEGACY) {
		return SC_CLEAR_FOCUS_SELECTION;
	}
	return name;
}

QVector<FunctionCall> IndigoSequenceParser::parse(QString code) const {
	const char lpMarker[] = "&<L*";
	const char rpMarker[] = "&>R*";
	const char commaMarker[] = "&C*";


	QString processedCode = "";
	QStringList lines = code.split('\n');
	for (const QString& line : lines) {
		// skip regular comments (not a //* - omitted item)
		if (line.trimmed().startsWith("//") && !line.trimmed().startsWith("//*")) {
			continue;
		}
		processedCode += line + "\n";
	}

	// replace parentheses and commas in strings with markers as they confuse
	// the parser (commas would otherwise split a single quoted argument into
	// multiple parameters, e.g. set_fits_header("OBJECT", "M31, Andromeda"))
	QRegularExpression stringRe(R"(\"[^\"]*\"|\'[^\']*\')");
	QRegularExpressionMatchIterator stringIt = stringRe.globalMatch(processedCode);

	while (stringIt.hasNext()) {
		QRegularExpressionMatch stringMatch = stringIt.next();
		QString originalStr = stringMatch.captured(0);
		QString replacedStr = originalStr;
		replacedStr.replace("(", lpMarker).replace(")", rpMarker).replace(",", commaMarker);
		processedCode.replace(originalStr, replacedStr);
	}

	QVector<FunctionCall> calls;
	// Updated regex pattern to capture optional "//*" at the beginning
	// The repeat body is matched with a recursive balanced-brace subpattern
	// ((?&brace), defined at the end) so that repeat blocks nested inside other
	// repeat blocks are captured whole instead of terminating at the first '}'.
	QRegularExpression re(R"((?:\/\/\*)?\s*((?:\w+)\.(?:\w+)\((?:[^)]*)\);|(?:\w+)\.repeat\((?:\d+),\s*function\s*\(\)\s*(?&brace)\);|var\s+(?:\w+)\s*=\s*new\s+Sequence\((?:\"[^\"]*\")?\);)(?(DEFINE)(?<brace>\{(?:[^{}]|(?&brace))*\})))");
	QRegularExpressionMatchIterator it = re.globalMatch(processedCode);

	while (it.hasNext()) {
		QRegularExpressionMatch match = it.next();
		QString fullCall = match.captured(0);
		QString callBody = match.captured(1);

		bool isOmitted = fullCall.trimmed().startsWith("//*");

		// Now parse the call body using the original regex
		QRegularExpression callRe(R"((\w+)\.(\w+)\(([^)]*)\);|(\w+)\.repeat\((\d+),\s*function\s*\(\)\s*\{((?:[^{}]|(?&brace))*)\}\);|var\s+(\w+)\s*=\s*new\s+Sequence\((\"[^\"]*\")?\);(?(DEFINE)(?<brace>\{(?:[^{}]|(?&brace))*\})))");
		QRegularExpressionMatch callMatch = callRe.match(callBody);

		FunctionCall call;
		call.omitted = isOmitted;

		if (!callMatch.captured(4).isEmpty()) {
			// Repeat function call with lambda
			call.objectName = callMatch.captured(4);
			call.functionName = "repeat";
			call.parameters.append(callMatch.captured(5)); // The repeat count
			call.parameters.append("lambda"); // Placeholder for the lambda function

			QString nestedCode = callMatch.captured(6).trimmed();
			call.nestedCalls = parse(nestedCode);

			// If the parent repeat is omitted, mark all nested calls as omitted
			if (isOmitted) {
				for (auto& nestedCall : call.nestedCalls) {
					nestedCall.omitted = true;
				}
			}
		} else if (!callMatch.captured(7).isEmpty()) {
			// Sequence object creation
			call.objectName = callMatch.captured(7);
			call.functionName = "Sequence";
			if (!callMatch.captured(8).isEmpty()) {
				QString name = callMatch.captured(8);
				// restore markers so parentheses/commas in the name round-trip
				name.replace(lpMarker, "(").replace(rpMarker, ")").replace(commaMarker, ",");
				call.parameters.append(name);
			}
		} else {
			// Regular function call
			call.objectName = callMatch.captured(1);
			call.functionName = resolve_legacy_name(callMatch.captured(2));
			QString params = callMatch.captured(3);
			call.parameters = params.split(',', QT_SKIP_EMPTY_PARTS);
			for (QString& param : call.parameters) {
				param = param.trimmed();
				// restore parentheses and commas in parameters
				param.replace(lpMarker, "(").replace(rpMarker, ")").replace(commaMarker, ",");
			}
		}

		calls.append(call);
	}

	return calls;
}

bool IndigoSequenceParser::validateCalls(const QVector<FunctionCall>& calls) const {
	const auto& widgetTypeMap = SequenceItemModel::instance().getWidgetTypes();

	for (const FunctionCall& call : calls) {
		if (call.functionName == "start") {
			if (call.parameters.size() != 0) {
				emit validationError(QString("start() does not accept parameters"));
				return false;
			}
			continue;
		}

		if (call.functionName == "Sequence") {
			if (call.parameters.size() > 1) {
				emit validationError(QString("Sequence() accepts 0 or 1 parameter, got %1").arg(call.parameters.size()));
				return false;
			}
			continue;
		}

		if (call.functionName == "repeat") {
			if (!validateCalls(call.nestedCalls)) {
				return false;
			}
			if (call.parameters.size() != 2) {
				emit validationError(QString("repeat() accepts 2 parameters, got %1").arg(call.parameters.size()));
				return false;
			}
			continue;
		}

		if (!widgetTypeMap.contains(call.functionName)) {
			emit validationError(QString("%1() is not a valid function").arg(call.functionName));
			return false;
		}

		const auto& widgetInfo = widgetTypeMap[call.functionName];
		if (call.parameters.size() != widgetInfo.parameters.size()) {
			emit validationError(QString("%1() accepts %2 parameters, got %3")
				.arg(call.functionName)
				.arg(widgetInfo.parameters.size())
				.arg(call.parameters.size()));
			return false;
		}
	}

	return true;
}

QString IndigoSequenceParser::generate(const QVector<FunctionCall>& calls, int indent) const {
	QString script;
	QString indentStr(indent, '\t');

	for (const FunctionCall& call : calls) {
		QString linePrefix = call.omitted ? indentStr + "//* " : indentStr;

		if (call.functionName == "repeat") {
			script += linePrefix + call.objectName + "." + call.functionName + "(" + call.parameters[0] + ", function() {\n";

			// If this repeat is disabled, all nested calls should be generated as disabled
			if (call.omitted) {
				// Create a temporary copy of nested calls with all marked as disabled
				QVector<FunctionCall> disabledNestedCalls = call.nestedCalls;
				for (auto& nestedCall : disabledNestedCalls) {
					nestedCall.omitted = true;
				}
				script += generate(disabledNestedCalls, indent + 1);
			} else {
				// Generate nested calls with the same indentation
				script += generate(call.nestedCalls, indent + 1);
			}

			script += linePrefix + "});\n";
		} else if (call.functionName == "Sequence") {
			// Constructor call
			script += linePrefix + "var " + call.objectName + " = new Sequence(" + call.parameters.join(", ").trimmed() + ");\n";
		} else {
			// Regular method call
			script += linePrefix + call.objectName + "." + call.functionName + "(" + call.parameters.join(", ").trimmed() + ");\n";
		}
	}

	return script;
}
