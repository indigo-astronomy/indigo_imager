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

#ifndef __SEXAGESIMALCONVERTER_H
#define __SEXAGESIMALCONVERTER_H

#include <QString>
#include <QRegularExpression>

class SexagesimalConverter {
public:
	static double stringToDouble(const QString& str, bool* ok = nullptr);
	static QString doubleToString(double value, int decimals = 2, bool showPlusSign = false);

private:
	static bool validateComponents(int degrees, int minutes, double seconds, QString& errorMsg);
	static const QRegularExpression fullFormat;    // dd:mm:ss.ff
	static const QRegularExpression fullNoDecFormat;   // dd:mm:ss
	static const QRegularExpression shortFormat;  // dd:mm.ff
	static const QRegularExpression shortNoDecFormat;   // dd:mm
	static const QRegularExpression decimalFormat; // dd.ff

	static const int MAX_MINUTES = 59;
	static const int MAX_SECONDS = 59;
};

#endif // __SEXAGESIMALCONVERTER_H