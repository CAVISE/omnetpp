%{
#include "bigdecimal.h"
#include "jprogressmonitor.h"
%}

%exception {
    try {
        $action
    } catch (std::exception& e) {
        SWIG_JavaThrowException(jenv, SWIG_JavaRuntimeException, const_cast<char*>(e.what()));
        return $null;
    }
}

/*--------------------------------------------------------------------------
 * int32 <--> int mapping
 *--------------------------------------------------------------------------*/
%include "intxtypes.h"

%typemap(jni)    int32_t "jint"
%typemap(jtype)  int32_t "int"
%typemap(jstype) int32_t "int"
%typemap(javain) int32_t "$javainput"
%typemap(javaout) int32_t {
   return $jnicall;
}

/*--------------------------------------------------------------------------
 * int64 <--> long mapping
 *--------------------------------------------------------------------------*/
%typemap(jni)    int64_t "jlong"
%typemap(jtype)  int64_t "long"
%typemap(jstype) int64_t "long"
%typemap(javain) int64_t "$javainput"
%typemap(javaout) int64_t {
   return $jnicall;
}

/*--------------------------------------------------------------------------
 * IProgressMonitor
 *--------------------------------------------------------------------------*/
%typemap(jni)    IProgressMonitor * "jobject"
%typemap(jtype)  IProgressMonitor * "org.eclipse.core.runtime.IProgressMonitor"
%typemap(jstype) IProgressMonitor * "org.eclipse.core.runtime.IProgressMonitor"
%typemap(javain) IProgressMonitor * "$javainput"
%typemap(in) IProgressMonitor * (JniProgressMonitor jProgressMonitor) {
   if ($input)
   {
      jProgressMonitor = JniProgressMonitor($input, jenv);
      $1 = &jProgressMonitor;
   }
   else
   {
      $1 = NULL;
   }
}

/*
 * Macro to add equals() and hashCode() to generated Java classes
 */
%define ADD_CPTR_EQUALS_AND_HASHCODE(CLASS)
%typemap(javacode) CLASS %{
  public boolean equals(Object obj) {
    if (this == obj)
      return true;
    if (obj == null || this.getClass() != obj.getClass())
      return false;
    return getCPtr(this) == getCPtr((CLASS)obj);
  }

  public int hashCode() {
    return (int)getCPtr(this);
  }
%}
%enddef

/*
 * ILock
 */
%define USE_COMMON_ENGINE_ILOCK()
class ILock;
%typemap(jstype) ILock "org.omnetpp.common.engine.ILock";
%typemap(javaout) ILock {
    long cPtr = $jnicall;
    return (cPtr == 0) ? null : new org.omnetpp.common.engine.ILock(cPtr, $owner);
}

%typemap(jstype) ILock* "org.omnetpp.common.engine.ILock";
%typemap(javaout) ILock* {
    long cPtr = $jnicall;
    return (cPtr == 0) ? null : new org.omnetpp.common.engine.ILock(cPtr, $owner);
}

%typemap(jstype) ILock& "org.omnetpp.common.engine.ILock";
%typemap(javaout) ILock& {
    long cPtr = $jnicall;
    return (cPtr == 0) ? null : new org.omnetpp.common.engine.ILock(cPtr, $owner);
}

%enddef