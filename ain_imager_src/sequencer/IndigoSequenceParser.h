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

signals:
	void validationError(QString errorMessage) const;
};

#endif // __INDIGOSEQUENCEPARSER_H