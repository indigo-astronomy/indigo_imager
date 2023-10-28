// Copyright (c) 2020 Rumen G.Bogdanovski
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

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>

#include <QDir>
#include <QString>
#include <QObject>

#include <utils.h>
#include <conf.h>


int get_number_of_cores() {
//#ifdef INDIGO_WINDOWS
//    SYSTEM_INFO sysinfo;
//    GetSystemInfo(&sysinfo);
//    return sysinfo.dwNumberOfProcessors;
//#else
	indigo_error("NUMCPU = %d", sysconf(_SC_NPROCESSORS_ONLN));
    return sysconf(_SC_NPROCESSORS_ONLN);
//#endif
}

void get_timestamp(char *timestamp_str) {
	assert(timestamp_str != nullptr);
	struct timeval tmnow;

	gettimeofday(&tmnow, NULL);
#if defined(INDIGO_WINDOWS)
	struct tm *lt;
	time_t rawtime;
	lt = localtime((const time_t *) &(tmnow.tv_sec));
	if (lt == NULL) {
		time(&rawtime);
		lt = localtime(&rawtime);
	}
	strftime(timestamp_str, 255, "%Y-%m-%d %H:%M:%S", lt);
#else
	strftime(timestamp_str, 255, "%Y-%m-%d %H:%M:%S", localtime((const time_t *) &tmnow.tv_sec));
#endif
	snprintf(timestamp_str + strlen(timestamp_str), 255, ".%03ld", tmnow.tv_usec/1000);
}

void get_date(char *date_str) {
	assert(date_str != nullptr);
	struct timeval tmnow;

	gettimeofday(&tmnow, NULL);
#if defined(INDIGO_WINDOWS)
	struct tm *lt;
	time_t rawtime;
	lt = localtime((const time_t *) &(tmnow.tv_sec));
	if (lt == NULL) {
		time(&rawtime);
		lt = localtime(&rawtime);
	}
	strftime(date_str, 255, "%Y-%m-%d", lt);
#else
	strftime(date_str, 255, "%Y-%m-%d", localtime((const time_t *) &tmnow.tv_sec));
#endif
}

#define HALF_DAY_SECONDS 43200
void get_date_jd(char *date_str) {
	assert(date_str != nullptr);
	struct timeval tmnow;

	gettimeofday(&tmnow, NULL);
	tmnow.tv_sec -= HALF_DAY_SECONDS;
#if defined(INDIGO_WINDOWS)
	struct tm *lt;
	time_t rawtime;
	lt = localtime((const time_t *) &(tmnow.tv_sec));
	if (lt == NULL) {
		time(&rawtime);
		rawtime -= HALF_DAY_SECONDS;
		lt = localtime(&rawtime);
	}
	strftime(date_str, 255, "%Y-%m-%d", lt);
#else
	strftime(date_str, 255, "%Y-%m-%d", localtime((const time_t *) &tmnow.tv_sec));
#endif
}

void get_time(char *time_str) {
	assert(time_str != nullptr);
	struct timeval tmnow;

	gettimeofday(&tmnow, NULL);
#if defined(INDIGO_WINDOWS)
	struct tm *lt;
	time_t rawtime;
	lt = localtime((const time_t *) &(tmnow.tv_sec));
	if (lt == NULL) {
		time(&rawtime);
		lt = localtime(&rawtime);
	}
	strftime(time_str, 255, "%H:%M:%S", lt);
#else
	strftime(time_str, 255, "%H:%M:%S", localtime((const time_t *) &tmnow.tv_sec));
#endif
	snprintf(time_str + strlen(time_str), 255, ".%03ld", tmnow.tv_usec/1000);
}

void get_current_output_dir(char *output_dir, char *prefix) {
	assert(output_dir != nullptr);
	QString path_prefix = QDir::homePath();

	if (prefix != nullptr && prefix[0] != '\0') path_prefix = QString(prefix);

	if (path_prefix.length() > 0) {
		char date_str[255] = {0};
		get_date_jd(date_str);
		if (!path_prefix.endsWith("/")) {
			path_prefix = path_prefix + QString("/");
		}
		if (!path_prefix.endsWith("/ain_data/")) {
			path_prefix = path_prefix + QString("ain_data/");
		}
		QString qlocation = QDir::toNativeSeparators(path_prefix + QString(date_str) + QString("/"));
		QDir dir = QDir::root();
		dir.mkpath(qlocation);
		strncpy(output_dir, qlocation.toUtf8().constData(), PATH_LEN);
	} else {
		if (!getcwd(output_dir, sizeof(output_dir))) {
			output_dir[0] = '\0';
		}
	}
}

void get_indigo_device_domain(char *device_domain, const char *device_name) {
	char *at = strrchr((char*)device_name, '@');
	if (at && at[1] != '\0') {
		strncpy(device_domain, &at[2], INDIGO_NAME_SIZE);
	} else {
		device_domain[0] = '\0';
	}
}

void remove_indigo_device_domain(char *device_name, int levels) {
	while (levels--) {
		char *at = strrchr(device_name, '@');
		if (at && at > device_name) {
			*(at-1) = '\0';
		} else {
			return;
		}
	}
}

void add_indigo_device_domain(char *device_name, const char *domain_name) {
	strcat(device_name, " @ ");
	strcat(device_name, domain_name);
}
