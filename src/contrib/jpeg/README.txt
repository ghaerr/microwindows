To enable jpeg support in nanogui:

- Download libjpeg source tarball at http://www.ijg.org
- Untar the tarball with tar -zxf <tarball>

In the jpeg directory, setup your compilation environment.
Here are the typical steps for linux:

- cp jconfig.doc jconfig.h
- cp makefile.ansi Makefile
- Open the Makefile and delete the jconfig.h: target
- Don't forget to setup the right cc tools if you cross-compile!
- type 'make'

In the config file:

- Enable the HAVE_JPEG_SUPPORT definition
- set the INCJPEG to the jpeg directory
- set the LIBJPEG to the jpeg directory
- compile the project

See demo3 if you want to see an example demo.

Martin Jolcoeur
martinj@visuaide.com
