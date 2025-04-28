// Copyright (c) 2019 Rumen G.Bogdanovski
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


#include "qindigoservice.h"

QIndigoService::QIndigoService(QByteArray name, QByteArray host, int port) :
	m_name(name),
	m_host(host),
	m_port(port),
	m_server_entry(nullptr),
	is_auto_service(false),
	auto_connect(true),
#ifdef INDIGO_VERSION_3
	prev_handle(NULL) {
#else
	prev_socket(-1) {
#endif
}

QIndigoService::QIndigoService(QByteArray name, QByteArray host, int port, bool connect, bool is_manual_service) :
	m_name(name),
	m_host(host),
	m_port(port),
	m_server_entry(nullptr),
	is_auto_service(!is_manual_service),
	auto_connect(connect),
#ifdef INDIGO_VERSION_3
	prev_handle(NULL) {
#else
	prev_socket(-1) {
#endif
}


QIndigoService::~QIndigoService() {
}


bool QIndigoService::connect() {
	int i = 5; /* 0.5 seconds */
#ifdef INDIGO_VERSION_3
	prev_handle = NULL;
#else
	prev_socket = -1;
#endif

	indigo_debug("%s(): %s %s %d\n",__FUNCTION__, m_name.constData(), m_host.constData(), m_port);
	indigo_result res = indigo_connect_server(m_name.constData(), m_host.constData(), m_port, &m_server_entry);
	indigo_debug("%s(): %s %s %d server_entry=%p\n",__FUNCTION__, m_name.constData(), m_host.constData(), m_port, m_server_entry);
	if (res != INDIGO_OK) return false;
	while (!connected() && i--) {
		indigo_usleep(100000);
	}
	indigo_debug("%s(): %s %s %d -> m_server_entry=%p\n",__FUNCTION__, m_name.constData(), m_host.constData(), m_port, m_server_entry);
	return connected();
}


bool QIndigoService::connected() const {
	if (m_server_entry) {
		return indigo_connection_status(m_server_entry, NULL);
	}
	indigo_debug("%s(): socket is null\n", __FUNCTION__);
	return false;
}


bool QIndigoService::disconnect() {
	indigo_debug("%s(): called m_server_entry= %p\n",__FUNCTION__, m_server_entry);
	if (m_server_entry) {
		indigo_debug("%s(): %s %s %d\n",__FUNCTION__, m_name.constData(), m_host.constData(), m_port);
		bool res = (indigo_disconnect_server(m_server_entry) == INDIGO_OK);
		while (connected()) {
			indigo_usleep(100000);
		}
		m_server_entry=nullptr;
		return res;
	}
	return false;
}
