// Copyright (c) 2021 Rumen G.Bogdanovski & David Hulse
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


#include "textdialog.h"

TextDialog::TextDialog(QString title, QWidget *parent): QDialog(parent)
{
	setWindowTitle(title);

	m_text = new QTextEdit;
	QFont font("monospace");
	font.setStyleHint(QFont::Monospace);
	m_text->setFont(font);
	m_text->setReadOnly(true);
	m_text->setTextColor(Qt::white);

	m_button_box = new QDialogButtonBox;
	m_close_button = m_button_box->addButton(tr("Close"), QDialogButtonBox::ActionRole);

	QVBoxLayout* viewLayout = new QVBoxLayout;
	viewLayout->setContentsMargins(0, 0, 0, 0);
	viewLayout->addWidget(m_text);

	QHBoxLayout* horizontalLayout = new QHBoxLayout;
	horizontalLayout->addWidget(m_button_box);

	QVBoxLayout* mainLayout = new QVBoxLayout;
	mainLayout->setContentsMargins(5, 5, 5, 5);
	mainLayout->addLayout(viewLayout);
	mainLayout->addLayout(horizontalLayout);

	setLayout(mainLayout);
	setMinimumWidth(700);
	setMinimumHeight(550);

	QObject::connect(m_close_button, SIGNAL(clicked()), this, SLOT(onClose()));
}

void TextDialog::scrollBottom() {
	m_text->verticalScrollBar()->setValue(m_text->verticalScrollBar()->maximum());
}

void TextDialog::scrollTop() {
	m_text->verticalScrollBar()->setValue(m_text->verticalScrollBar()->minimum());
}

void TextDialog::onClose() {
	close();
}
