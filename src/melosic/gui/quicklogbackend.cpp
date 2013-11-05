/**************************************************************************
**  Copyright (C) 2013 Christian Manning
**
**  This program is free software: you can redistribute it and/or modify
**  it under the terms of the GNU General Public License as published by
**  the Free Software Foundation, either version 3 of the License, or
**  (at your option) any later version.
**
**  This program is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**  GNU General Public License for more details.
**
**  You should have received a copy of the GNU General Public License
**  along with this program.  If not, see <http://www.gnu.org/licenses/>.
**************************************************************************/

#include <QtQuick/private/qquicktextedit_p.h>

#include "quicklogbackend.hpp"

namespace Melosic {

QuickLogBackend::QuickLogBackend(QObject *parent) :
    QObject(parent)
{
    connect(this, &QuickLogBackend::append, [this] (QString str) {
        if(!m_text_edit)
            return;
        m_text_edit->append(str);
    });
}

void QuickLogBackend::consume(const logging::record_view&, const string_type& str) {
    Q_EMIT append(QString::fromStdString(str));
}

QObject* QuickLogBackend::textEdit() const {
    return m_text_edit;
}

void QuickLogBackend::setTextEdit(QObject* te) {
    if(!te)
        return;

    m_text_edit = te->findChild<QQuickTextEdit*>();
    assert(m_text_edit);

    connect(m_text_edit, &QQuickTextEdit::destroyed, [this] (QObject*) {
        m_text_edit = nullptr;
    });

    Q_EMIT textEditChanged(m_text_edit);
}

} // namespace Melosic
