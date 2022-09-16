// Copyright (c) 2022 Rumen G.Bogdanovski
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

#include <indigo/indigo_bus.h>
#include <indigo/indigo_md5.h>
#include <syncutils.h>
#include <QDir>
#include <QDebug>
#include <QDirIterator>

void SyncUtils::rebuild() {
	clear();
	QDirIterator it(m_work_dir, { "*" }, QDir::Files, QDirIterator::Subdirectories);
	while (it.hasNext()) {
		QString file_name = it.next();
		if (file_name.endsWith(".log")) continue;
		add(file_name);
	}
}
bool SyncUtils::needs_sync(QString file) {
	QStringList pieces = file.split( "_" );
	QString last = pieces.value(pieces.length() - 1);
	pieces = last.split( "." );
	QString hash = pieces.value(0);
	return !m_digests.contains(hash) && (hash.length() == 32);
}

void SyncUtils::add(QString file) {
	char digest[33] = {0};
	char file_name[PATH_MAX];
	strcpy(file_name, file.toUtf8().constData());
	FILE *fp = fopen(file_name, "rb");
	if (fp) {
		indigo_md5_file_partial(digest, fp, INDIGO_PARTIAL_MD5_LEN);
		fclose(fp);
		m_digests.insert(digest, file_name);
		indigo_debug("%s -> %s", digest, file_name);
	}
}

void SyncUtils::clear() {
	m_digests.clear();
}
