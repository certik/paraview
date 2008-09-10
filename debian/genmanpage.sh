#! /usr/bin/zsh
#set -x
for name in H5detect ProcessShader csClient csServer kwProcessXML mkg3states pvbatch pvdataserver pvpython pvrenderserver pvserver vtkParseOGLExt vtkSMExtractDocumentation vtkVREncodeString vtkWrapClientServer DobranoViz; do

manname=`echo -n ${name}.1`
cat > $manname <<EOF
.TH NAME 1
.\" NAME $name, SECTION 1
.SH NAME
$name \-
.SH SYNOPSIS
.B $name
.br
.SH DESCRIPTION
This manual page documents briefly the
.BR $name
command.

You can find information in the paraview manual page and on the
paraview web page http://www.paraview.org.

.SH AUTHOR
The Debian Scientific Computing Team
EOF
echo debian/$manname /usr/share/man/man1
done

