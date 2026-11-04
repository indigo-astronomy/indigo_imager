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


#include <QByteArray>
#include <QFile>
#include <QMutex>
#include <QMutexLocker>
#include <windows.h>
#include <wininet.h>
#include <wininetdownload.h>

#define DOWNLOAD_BUFFER_SIZE (64 * 1024)

static QMutex session_mutex;
static HINTERNET session = NULL;

// closing the session interrupts a blocking call on its handles, only the first caller closes it
static bool close_session() {
	QMutexLocker locker(&session_mutex);
	if (session == NULL) {
		return false;
	}
	InternetCloseHandle(session);
	session = NULL;
	return true;
}

void wininet_cancel() {
	close_session();
}

QString wininet_download(const QString &url, const QString &path, volatile bool *aborted) {
	HINTERNET opened = InternetOpenA("Ain Platesolver", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
	if (opened == NULL) {
		return QString("InternetOpen() failed (%1)").arg(GetLastError());
	}
	{
		QMutexLocker locker(&session_mutex);
		session = opened;
	}
	QString error;
	HINTERNET request = NULL;
	if (*aborted) {
		error = "Aborted";
	} else {
		request = InternetOpenUrlA(opened, url.toUtf8().constData(), NULL, 0, INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE, 0);
		if (request == NULL) {
			error = QString("InternetOpenUrl() failed (%1)").arg(GetLastError());
		}
	}
	if (request != NULL) {
		DWORD status = 0, status_size = sizeof(status);
		if (!HttpQueryInfoA(request, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &status, &status_size, NULL) || status != 200) {
			error = QString("HTTP status %1").arg((unsigned long)status);
		} else {
			QFile file(path);
			if (!file.open(QIODevice::WriteOnly)) {
				error = QString("Failed to create %1 (%2)").arg(path, file.errorString());
			} else {
				QByteArray buffer(DOWNLOAD_BUFFER_SIZE, 0);
				while (true) {
					DWORD bytes_read = 0;
					if (!InternetReadFile(request, buffer.data(), DOWNLOAD_BUFFER_SIZE, &bytes_read)) {
						error = QString("InternetReadFile() failed (%1)").arg(GetLastError());
						break;
					}
					if (bytes_read == 0) {
						break;
					}
					if (file.write(buffer.constData(), bytes_read) != (qint64)bytes_read) {
						error = QString("Failed to write %1 (%2)").arg(path, file.errorString());
						break;
					}
					if (*aborted) {
						error = "Aborted";
						break;
					}
				}
				file.close();
			}
		}
	}
	// a canceled session has closed its request too
	{
		QMutexLocker locker(&session_mutex);
		if (session != NULL) {
			if (request != NULL) {
				InternetCloseHandle(request);
			}
			InternetCloseHandle(session);
			session = NULL;
		}
	}
	return error;
}
