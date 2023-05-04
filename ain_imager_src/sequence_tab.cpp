// Copyright (c) 2023 Rumen G.Bogdanovski
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

#include "imagerwindow.h"
#include "propertycache.h"
#include <conf.h>
#include <utils.h>

void ImagerWindow::create_sequence_tab(QFrame *sequence_frame) {
	/*
	QGridLayout *capture_frame_layout = new QGridLayout();
	capture_frame_layout->setAlignment(Qt::AlignTop);
	capture_frame->setLayout(capture_frame_layout);
	capture_frame->setFrameShape(QFrame::StyledPanel);
	capture_frame->setMinimumWidth(CAMERA_FRAME_MIN_WIDTH);
	capture_frame->setContentsMargins(0, 0, 0, 0);
	*/
}

void ImagerWindow::on_sequence_updated() {
	// TESTCODE
	QtConcurrent::run([=]() {
		indigo_debug("CALLED: %s\n", __FUNCTION__);
		static char selected_agent[INDIGO_NAME_SIZE];
		get_selected_imager_agent(selected_agent);
		static QList<QString> batches;
		static QString sequence;
		m_sequence_editor->generate_sequence(sequence, batches);

		indigo_error("Sequence: %s\n", sequence.toStdString().c_str());
		for (int i = 0; i < batches.count(); i++) {
			indigo_error("BATCH %d: %s\n", i+1, batches[i].toStdString().c_str());
		}

		change_imager_agent_sequence(selected_agent, sequence, batches);
	});
}