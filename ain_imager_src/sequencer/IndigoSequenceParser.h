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

#ifndef __INDIGOSEQUENCEPARSER_H
#define __INDIGOSEQUENCEPARSER_H

#include <QString>
#include <QStringList>
#include <QVector>
#include <QObject>

struct FunctionCall {
	bool omitted;
	QString objectName;
	QString functionName;
	QStringList parameters;
	QVector<FunctionCall> nestedCalls;
};

class IndigoSequenceParser : public QObject {
	Q_OBJECT
public:
	QVector<FunctionCall> parse(QString code) const;
	bool validateCalls(const QVector<FunctionCall>& calls) const;
	QString generate(const QVector<FunctionCall>& calls, int indent = 0) const;

	/* The code that the last parse() skipped because it is not a sequence call,
	   such as a variable or an if statement. Loading such code into the editor
	   would lose it. */
	const QStringList& unparsedCode() const { return m_unparsed; }

	/* Compares two parsed sequences by their steps and parameter values, so that
	   "5" equals "5.0" and "00:42:44.3" equals 0.712306. The constructor is not
	   compared. Returns an empty string when they are equal, or the difference. */
	static QString compareCalls(const QVector<FunctionCall>& a, const QVector<FunctionCall>& b);

signals:
	void validationError(QString errorMessage) const;

private:
	QVector<FunctionCall> parseCode(QString code) const;
	mutable QStringList m_unparsed;
};

#endif // __INDIGOSEQUENCEPARSER_H