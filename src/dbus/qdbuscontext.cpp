/***********************************************************************
*
* Copyright (c) 2012-2025 Barbara Geller
* Copyright (c) 2012-2025 Ansel Sermersheim
*
* Copyright (c) 2015 The Qt Company Ltd.
* Copyright (c) 2012-2016 Digia Plc and/or its subsidiary(-ies).
* Copyright (c) 2008-2012 Nokia Corporation and/or its subsidiary(-ies).
*
* This file is part of CopperSpice.
*
* CopperSpice is free software. You can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public License
* version 2.1 as published by the Free Software Foundation.
*
* CopperSpice is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*
* https://www.gnu.org/licenses/
*
***********************************************************************/

#include "qdbusmessage.h"
#include "qdbusconnection.h"
#include "qdbusabstractadaptor.h"

#include "qdbuscontext.h"
#include "qdbuscontext_p.h"

#ifndef QT_NO_DBUS

QT_BEGIN_NAMESPACE

QDBusContextPrivate *QDBusContextPrivate::set(QObject *obj, QDBusContextPrivate *newContext)
{
    // determine if this is an adaptor or not
    if (qobject_cast<QDBusAbstractAdaptor *>(obj))
        obj = obj->parent();

    Q_ASSERT(obj);

    void *ptr = obj->qt_metacast("QDBusContext");
    QDBusContext *q_ptr = reinterpret_cast<QDBusContext *>(ptr);
    if (q_ptr) {
        QDBusContextPrivate *old = q_ptr->d_ptr;
        q_ptr->d_ptr = newContext;
        return old;
    }

    return 0;
}

/*!
    \since 4.3
    \class QDBusContext
    \inmodule QtDBus

    \brief The QDBusContext class allows slots to determine the D-Bus context of the calls.

    When a slot is called in an object due to a signal delivery or due
    to a remote method call, it is sometimes necessary to know the
    context in which that happened. In particular, if the slot
    determines that it wants to send the reply at a later opportunity
    or if it wants to reply with an error, the context is needed.

    The QDBusContext class is an alternative to accessing the context
    that doesn't involve modifying the code generated by the \l
    {QtDBus XML Compiler (qdbusxml2cpp)}.

    QDBusContext is used by subclassing it from the objects being
    exported using QDBusConnection::registerObject(). The following
    example illustrates the usage:

    \snippet doc/src/snippets/code/src_qdbus_qdbuscontext.cpp 0

    The example illustrates the two typical uses, that of sending
    error replies and that of delayed replies.

    Note: do not subclass QDBusContext and QDBusAbstractAdaptor at the
    same time. QDBusContext should appear in the real object, not the
    adaptor. If it's necessary from the adaptor code to determine the
    context, use a public inheritance and access the functions via
    QObject::parent().
*/

/*!
  Constructs an empty QDBusContext.
 */
QDBusContext::QDBusContext()
    : d_ptr(0)
{
}

/*!
  An empty destructor.
 */
QDBusContext::~QDBusContext()
{
}

/*!
    Returns true if we are processing a D-Bus call. If this function
    returns true, the rest of the functions in this class are
    available.

    Accessing those functions when this function returns false is
    undefined and may lead to crashes.
*/
bool QDBusContext::calledFromDBus() const
{
    return d_ptr;
}

/*!
    Returns the connection from which this call was received.
*/
QDBusConnection QDBusContext::connection() const
{
    return d_ptr->connection;
}

/*!
    Returns the message that generated this call.
*/
const QDBusMessage &QDBusContext::message() const
{
    return d_ptr->message;
}

/*!
    Returns true if this call will have a delayed reply.

    \sa setDelayedReply()
*/
bool QDBusContext::isDelayedReply() const
{
    return message().isDelayedReply();
}

/*!
    Sets whether this call will have a delayed reply or not.

    If \a enable is false, QtDBus will automatically generate a reply
    back to the caller, if needed, as soon as the called slot returns.

    If \a enable is true, QtDBus will not generate automatic
    replies. It will also ignore the return value from the slot and
    any output parameters. Instead, the called object is responsible
    for storing the incoming message and send a reply or error at a
    later time.

    Failing to send a reply will result in an automatic timeout error
    being generated by D-Bus.
*/
void QDBusContext::setDelayedReply(bool enable) const
{
    message().setDelayedReply(enable);
}

/*!
    Sends an error \a name as a reply to the caller. The optional \a
    msg parameter is a human-readable text explaining the failure.

    If an error is sent, the return value and any output parameters
    from the called slot will be ignored by QtDBus.
*/
void QDBusContext::sendErrorReply(const QString &name, const QString &msg) const
{
    setDelayedReply(true);
    connection().send(message().createErrorReply(name, msg));
}

/*!
    \overload
    Sends an error \a type as a reply to the caller. The optional \a
    msg parameter is a human-readable text explaining the failure.

    If an error is sent, the return value and any output parameters
    from the called slot will be ignored by QtDBus.
*/
void QDBusContext::sendErrorReply(QDBusError::ErrorType type, const QString &msg) const
{
    setDelayedReply(true);
    connection().send(message().createErrorReply(type, msg));
}

QT_END_NAMESPACE

#endif // QT_NO_DBUS
