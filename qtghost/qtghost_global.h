#ifndef QTGHOST_GLOBAL_H
#define QTGHOST_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(QTGHOST_LIBRARY)
#  define QTGHOSTSHARED_EXPORT Q_DECL_EXPORT
#else
#  define QTGHOSTSHARED_EXPORT Q_DECL_IMPORT
#endif

#endif // QTGHOST_GLOBAL_H
