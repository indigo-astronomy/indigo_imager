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


QVector<FunctionCall> IndigoSequenceParser::parse(QString code) const {
	const char lpMarker[] = "&<L*";
	const char rpMarker[] = "&>R*";

	// remove comments
	QRegularExpression commentRe(R"(//.*$)");
	code.remove(commentRe);

	// replace parentheses in strings with markers as they confuse parser
	QString processedCode = code;
	QRegularExpression stringRe(R"(\"[^\"]*\"|\'[^\']*\')");
	QRegularExpressionMatchIterator stringIt = stringRe.globalMatch(code);

	while (stringIt.hasNext()) {
		QRegularExpressionMatch stringMatch = stringIt.next();
		QString originalStr = stringMatch.captured(0);
		QString replacedStr = originalStr;
		replacedStr.replace("(", lpMarker).replace(")", rpMarker);
		processedCode.replace(originalStr, replacedStr);
	}

	QVector<FunctionCall> calls;
	QRegularExpression re(R"((\w+)\.(\w+)\(([^)]*)\);|(\w+)\.repeat\((\d+),\s*function\s*\(\)\s*\{([^}]*)\}\);|var\s+(\w+)\s*=\s*new\s+Sequence\((\"[^\"]*\")?\);)");
	QRegularExpressionMatchIterator it = re.globalMatch(processedCode);

	while (it.hasNext()) {
		QRegularExpressionMatch match = it.next();
		FunctionCall call;

		if (!match.captured(4).isEmpty()) {
			// Repeat function call with lambda
			call.objectName = match.captured(4);
			call.functionName = "repeat";
			call.parameters.append(match.captured(5)); // The repeat count
			call.parameters.append("lambda"); // Placeholder for the lambda function

			QString nestedCode = match.captured(6).trimmed();
			call.nestedCalls = parse(nestedCode);
		} else if (!match.captured(7).isEmpty()) {
			// Sequence object creation
			call.objectName = match.captured(7);
			call.functionName = "Sequence";
			if (!match.captured(8).isEmpty()) {
				call.parameters.append(match.captured(8));
			}
		} else {
			// Regular function call
			call.objectName = match.captured(1);
			call.functionName = match.captured(2);
			QString params = match.captured(3);
			call.parameters = params.split(',', QT_SKIP_EMPTY_PARTS);
			for (QString& param : call.parameters) {
				param = param.trimmed();
				// restore parentheses in parameters
				param.replace(lpMarker, "(").replace(rpMarker, ")");
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
		if (call.functionName == "repeat") {
			script += indentStr + call.objectName + "." + call.functionName + "(" + call.parameters[0] + ", function() {\n";
			script += generate(call.nestedCalls, indent + 1);
			script += indentStr + "});\n";
		} else if (call.functionName == "Sequence") {
			// Constructor call
			script += indentStr + "var " + call.objectName + " = new Sequence(" + call.parameters.join(", ").trimmed() + ");\n";
		} else {
			// Regular method call
			script += indentStr + call.objectName + "." + call.functionName + "(" + call.parameters.join(", ").trimmed() + ");\n";
		}
	}

	return script;
}
