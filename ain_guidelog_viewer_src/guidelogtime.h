// Copyright (c) 2026 Rumen G.Bogdanovski
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

#ifndef GUIDELOGTIME_H
#define GUIDELOGTIME_H

#include <QtGlobal>

class QDateTime;
class QString;

// Guide-log timestamp parsing.
//
// Log timestamps are written in one fixed layout ("yyyy-MM-dd HH:mm:ss.zzz",
// the fractional part optional, the whole field optionally quoted), so they are
// decoded by hand rather than through QDateTime::fromString(). That is not a
// micro-optimisation: fromString() with a format string builds a QDateTimeParser
// and resolves a time zone on every call (~91 us here), which at one call per
// row costs seconds on a long session — and the periodic-error tools re-parse
// the whole session every time the graph's visible window changes.
//
// Timestamps are read as a plain wall-clock reading with no zone attached. The
// PE tools only ever use differences between them, and treating them as local
// time would make an elapsed-seconds axis jump by an hour across a daylight
// saving transition in the middle of a session.
namespace GuideLogTime {

// Parses value into milliseconds since 1970-01-01 00:00:00, reading the text as
// UTC (i.e. as-written). Returns false, leaving *msecs untouched, if the field
// does not hold a timestamp. Falls back to QDateTime::fromString() for input
// that does not match the expected fixed layout.
bool parseMSecs(const QString &value, qint64 *msecs);

// QDateTime flavour of parseMSecs(), for the places that want to format a
// timestamp rather than measure with it. The result carries the UTC time zone,
// so toString() prints back the same digits the log held. Returns an invalid
// QDateTime on failure.
QDateTime parse(const QString &value);

} // namespace GuideLogTime

#endif // GUIDELOGTIME_H
