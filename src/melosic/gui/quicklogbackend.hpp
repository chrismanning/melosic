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

#ifndef MELOSIC_QUICKLOGBACKEND_HPP
#define MELOSIC_QUICKLOGBACKEND_HPP

#include <QObject>

#include <boost/log/sinks/basic_sink_backend.hpp>

#include <melosic/melin/logging.hpp>

class QQuickTextEdit;

namespace sinks = logging::sinks;

namespace Melosic {

class QuickLogBackend : public QObject,
        public sinks::basic_formatted_sink_backend<char, sinks::synchronized_feeding>
{
    Q_OBJECT
    Q_PROPERTY(QObject* textEdit READ textEdit WRITE setTextEdit NOTIFY textEditChanged)
    QQuickTextEdit* m_text_edit{nullptr};
public:
    explicit QuickLogBackend(QObject *parent = nullptr);

    void consume(const logging::record_view& rec, const string_type& str);
    QObject* textEdit() const;
    void setTextEdit(QObject*);

Q_SIGNALS:
    void textEditChanged(QObject*);
    void append(QString);
};

} // namespace Melosic

#endif // MELOSIC_QUICKLOGBACKEND_HPP
