//==========================================================================
//  STRUCTINSP.CC - part of
//
//                     OMNeT++/OMNEST
//            Discrete System Simulation in C++
//
//  Implementation of
//    cStructDescriptor-based inspector
//
//==========================================================================

/*--------------------------------------------------------------*
  Copyright (C) 1992-2005 Andras Varga

  This file is distributed WITHOUT ANY WARRANTY. See the file
  `license' for details on this and other legal matters.
*--------------------------------------------------------------*/

#include <memory>
#include <string.h>
#include <math.h>

#include "cstruct.h"

#include "tkapp.h"
#include "tklib.h"
#include "inspfactory.h"
#include "structinsp.h"


void _dummy_for_structinsp() {}



TStructPanel::TStructPanel(const char *widgetname, cPolymorphic *obj) :
    TInspectorPanel(widgetname,obj)
{
}

void TStructPanel::flushIfNeeded(int limit)
{
    // bump write pointer
    writeptr += strlen(writeptr);

    // flush if there are more than limit chars in the buffer
    if (writeptr-buf>=limit)
    {
        Tcl_Interp *interp = getTkApplication()->getInterp();
        TclQuotedString quotedstr(buf);
        CHK(Tcl_VarEval(interp, widgetname, ".txt insert end ", quotedstr.get(), " field", NULL));
        writeptr = buf; // reset writeptr to beginning of buffer
    }
}


void TStructPanel::displayStruct(cStructDescriptor *sd, int level)
{
   // print everything in a buffer, and periodically display it as the buffer gets full.
   // this is a temporary solution, should be replaced by something more professional!

   int indent = level*4;

   for (int fld=0; fld<sd->getFieldCount(); fld++)
   {
       int type = sd->getFieldType(fld);
       bool isarray = type==cStructDescriptor::FT_BASIC_ARRAY ||
                      type==cStructDescriptor::FT_SPECIAL_ARRAY ||
                      type==cStructDescriptor::FT_STRUCT_ARRAY;
       cStructDescriptor *sd1;

       if (!isarray)
       {
           switch(type)
           {
               case cStructDescriptor::FT_BASIC:
                   tmpbuf[0]='\0';
                   sd->getFieldAsString(fld, 0, tmpbuf, MAXWRITE);
                   if (sd->getFieldEnumName(fld))
                   {
                       // display enum value as int and as string
                       cEnum* enm = findEnum(sd->getFieldEnumName(fld));
                       if (enm)
                       {
                           int key = atol(tmpbuf);
                           sprintf(tmpbuf, "%d (%s)", key, enm->stringFor(key));
                       }
                   }
                   sprintf(writeptr,"%*s%s %s  =  %s\n", indent, "", sd->getFieldTypeString(fld), sd->getFieldName(fld), tmpbuf);
                   flushIfNeeded(FLUSHLIMIT);
                   break;
               case cStructDescriptor::FT_SPECIAL:
                   sprintf(writeptr,"%*s%s %s  =  ...\n", indent, "", sd->getFieldTypeString(fld), sd->getFieldName(fld)); //FIXME
                   flushIfNeeded(FLUSHLIMIT);
                   break;
               case cStructDescriptor::FT_STRUCT:
                   tmpbuf[0]='\0';
                   sd->getFieldAsString(fld, 0, tmpbuf, MAXWRITE);
                   sprintf(writeptr,"%*s%s %s  =  %s ", indent, "", sd->getFieldTypeString(fld), sd->getFieldName(fld), tmpbuf);
                   flushIfNeeded(FLUSHLIMIT);

                   sd1 = cStructDescriptor::createDescriptorFor(sd->getFieldStructName(fld),
                                                                sd->getFieldStructPointer(fld,0));
                   if (!sd1)
                   {
                       sprintf(writeptr, (tmpbuf[0] ? "\n" : "{...}\n"));
                       flushIfNeeded(FLUSHLIMIT);
                   }
                   else
                   {
                       sprintf(writeptr,"{\n");
                       flushIfNeeded(FLUSHLIMIT);
                       displayStruct(sd1,level+1);
                       delete sd1;
                       sprintf(writeptr,"%*s}\n", indent, "");
                       flushIfNeeded(FLUSHLIMIT);
                   }
                   break;
               default:
                   sprintf(writeptr,"%*s%s %s  =  (unknown type)\n", indent, "", sd->getFieldTypeString(fld), sd->getFieldName(fld));
                   flushIfNeeded(FLUSHLIMIT);
           }
       }
       else
       {
           int size = sd->getArraySize(fld);
           for (int i=0; i<size; i++)
           {
               switch(type)
               {
                   case cStructDescriptor::FT_BASIC_ARRAY:
                       sd->getFieldAsString(fld, i, tmpbuf, MAXWRITE); // FIXME: error handling!
                       if (sd->getFieldEnumName(fld))
                       {
                           // display enum value as int and as string
                           cEnum* enm = findEnum(sd->getFieldEnumName(fld));
                           if (enm)
                           {
                               int key = atol(tmpbuf);
                               sprintf(tmpbuf, "%d (%s)", key, enm->stringFor(key));
                           }
                       }
                       sprintf(writeptr,"%*s%s %s[%d]  =  %s\n", indent, "", sd->getFieldTypeString(fld), sd->getFieldName(fld), i, tmpbuf);
                       flushIfNeeded(FLUSHLIMIT);
                       break;
                   case cStructDescriptor::FT_SPECIAL_ARRAY:
                       sprintf(writeptr,"%*s%s %s[%d]  =  ...\n", indent, "", sd->getFieldTypeString(fld), sd->getFieldName(fld), i); //FIXME
                       flushIfNeeded(FLUSHLIMIT);
                       break;
                   case cStructDescriptor::FT_STRUCT_ARRAY:
                       tmpbuf[0]='\0';
                       sd->getFieldAsString(fld, i, tmpbuf, MAXWRITE);
                       sprintf(writeptr,"%*s%s %s[%d]  =  %s ", indent, "", sd->getFieldTypeString(fld), sd->getFieldName(fld), i, tmpbuf);
                       flushIfNeeded(FLUSHLIMIT);

                       sd1 = cStructDescriptor::createDescriptorFor(sd->getFieldStructName(fld),
                                                                    sd->getFieldStructPointer(fld,i));
                       if (!sd1)
                       {
                           sprintf(writeptr, (tmpbuf[0] ? "\n" : "{...}\n"));
                           flushIfNeeded(FLUSHLIMIT);
                       }
                       else
                       {
                           sprintf(writeptr,"{\n");
                           flushIfNeeded(FLUSHLIMIT);
                           displayStruct(sd1,level+1);
                           delete sd1;
                           sprintf(writeptr,"}\n");
                           flushIfNeeded(FLUSHLIMIT);
                       }
                       break;
                   default:
                       sprintf(writeptr,"%*s%s %s[%d]  =  (unknown type)\n", indent, "", sd->getFieldTypeString(fld), sd->getFieldName(fld), i);
                       flushIfNeeded(FLUSHLIMIT);
               }
           }
       }
   }
}

void TStructPanel::update()
{
   Tcl_Interp *interp = getTkApplication()->getInterp();

   // delete display
   CHK(Tcl_VarEval(interp, widgetname, ".txt delete 1.0 end", NULL));

   if (!object)
   {
       CHK(Tcl_VarEval(interp, widgetname, ".txt insert 1.0 {<none>}", NULL));
       return;
   }

   // get descriptor object
   cStructDescriptor *sd = object->createDescriptor();
   if (!sd)
   {
       CHK(Tcl_VarEval(interp, widgetname, ".txt insert 1.0 {class ", object->className()," {\n"
                               "    <fields cannot be displayed -- no descriptor object registered>\n"
                               "}}\n", NULL));
       return;
   }

   // display object and delete descriptor
   writeptr = buf;
   *writeptr = '\0';

   sprintf(writeptr,"class %s {\n", object->className());
   flushIfNeeded(FLUSHLIMIT);

   displayStruct(sd,1);
   delete sd;

   sprintf(writeptr,"}\n");

   // flush the rest
   flushIfNeeded(0);
}

void TStructPanel::writeBack()
{
   // nothing to do here as long as GUI is a textfield
}

int TStructPanel::inspectorCommand(Tcl_Interp *interp, int argc, const char **argv)
{
   //
   // These functions are currently not used. Might be useful for a Tcl-based struct inspector.
   //
   if (argc<1) {Tcl_SetResult(interp, TCLCONST("wrong argcount"), TCL_STATIC); return TCL_ERROR;}

   std::auto_ptr<cStructDescriptor> sd(object->createDescriptor());

   if (strcmp(argv[0],"count")==0)   // 'opp_inspectorcommand <inspector> fieldcount ...'
   {
      if (argc!=1) {Tcl_SetResult(interp, TCLCONST("wrong argcount"), TCL_STATIC); return TCL_ERROR;}
      char buf[20];
      sprintf(buf, "%d", sd->getFieldCount());
      Tcl_SetResult(interp, buf, TCL_VOLATILE);
      return TCL_OK;
   }

   if (strcmp(argv[0],"type")==0)   // 'opp_inspectorcommand <inspector> fieldtype ...'
   {
      if (argc!=2) {Tcl_SetResult(interp, TCLCONST("wrong argcount"), TCL_STATIC); return TCL_ERROR;}
      int fld = atoi(argv[1]);
      int type = sd->getFieldType(fld);
      switch(type)
      {
          case cStructDescriptor::FT_BASIC:
              Tcl_SetResult(interp, TCLCONST("basic"), TCL_STATIC); break;
          case cStructDescriptor::FT_SPECIAL:
              Tcl_SetResult(interp, TCLCONST("special"), TCL_STATIC); break;
          case cStructDescriptor::FT_STRUCT:
              Tcl_SetResult(interp, TCLCONST("struct"), TCL_STATIC); break;
          case cStructDescriptor::FT_BASIC_ARRAY:
              Tcl_SetResult(interp, TCLCONST("basic array"), TCL_STATIC); break;
          case cStructDescriptor::FT_SPECIAL_ARRAY:
              Tcl_SetResult(interp, TCLCONST("special array"), TCL_STATIC); break;
          case cStructDescriptor::FT_STRUCT_ARRAY:
              Tcl_SetResult(interp, TCLCONST("struct array"), TCL_STATIC); break;
          default:
              Tcl_SetResult(interp, TCLCONST("invalid"), TCL_STATIC);
      }
      return TCL_OK;
   }

   if (strcmp(argv[0],"name")==0)   // 'opp_inspectorcommand <inspector> fieldname <fldid> ...'
   {
      if (argc!=2) {Tcl_SetResult(interp, TCLCONST("wrong argcount"), TCL_STATIC); return TCL_ERROR;}
      int fld = atoi(argv[1]);
      Tcl_SetResult(interp, TCLCONST(sd->getFieldName(fld)), TCL_VOLATILE);
      return TCL_OK;
   }

   if (strcmp(argv[0],"typename")==0)   // 'opp_inspectorcommand <inspector> fieldtypename <fldid> ...'
   {
      if (argc!=2) {Tcl_SetResult(interp, TCLCONST("wrong argcount"), TCL_STATIC); return TCL_ERROR;}
      int fld = atoi(argv[1]);
      Tcl_SetResult(interp, TCLCONST(sd->getFieldTypeString(fld)), TCL_VOLATILE);
      return TCL_OK;
   }

   if (strcmp(argv[0],"arraysize")==0)   // 'opp_inspectorcommand <inspector> fieldarraysize <fldid> ...'
   {
      if (argc!=2) {Tcl_SetResult(interp, TCLCONST("wrong argcount"), TCL_STATIC); return TCL_ERROR;}
      int fld = atoi(argv[1]);
      char buf[20];
      sprintf(buf, "%d", sd->getArraySize(fld));
      Tcl_SetResult(interp, buf, TCL_VOLATILE);
      return TCL_OK;
   }

   if (strcmp(argv[0],"value")==0)   // 'opp_inspectorcommand <inspector> fieldvalue <fldid> ?index?...'
   {
      if (argc!=2 && argc!=3) {Tcl_SetResult(interp, TCLCONST("wrong argcount"), TCL_STATIC); return TCL_ERROR;}
      int fld = atoi(argv[1]);
      int i=0;
      if (argc==4)
         i = atoi(argv[2]);
      if (!sd->getFieldAsString(fld, i, tmpbuf, MAXWRITE))
      {
         Tcl_SetResult(interp, TCLCONST("error in getFieldAsString()"), TCL_STATIC);
         return TCL_ERROR;
      }
      Tcl_SetResult(interp, buf, TCL_VOLATILE);
      return TCL_OK;
   }

   if (strcmp(argv[0],"enumname")==0)   // 'opp_inspectorcommand <inspector> fieldenumname <fldid> ?index?...'
   {
      if (argc!=2) {Tcl_SetResult(interp, TCLCONST("wrong argcount"), TCL_STATIC); return TCL_ERROR;}
      int fld = atoi(argv[1]);
      Tcl_SetResult(interp, TCLCONST(sd->getFieldEnumName(fld)), TCL_VOLATILE);
      return TCL_OK;
   }

   if (strcmp(argv[0],"structname")==0)   // 'opp_inspectorcommand <inspector> fieldenumname <fldid> ?index?...'
   {
      if (argc!=2) {Tcl_SetResult(interp, TCLCONST("wrong argcount"), TCL_STATIC); return TCL_ERROR;}
      int fld = atoi(argv[1]);
      Tcl_SetResult(interp, TCLCONST(sd->getFieldStructName(fld)), TCL_VOLATILE);
      return TCL_OK;
   }
   return TCL_ERROR;
}

