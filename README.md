# qputty-qt5
A Qt 5 port for putty, the free telnet/ssh client.

This is a Qt 5 port for Qt 4 based [qputty project](http://sourceforge.net/projects/qputty/)

What was done:
- Qt 5 port
- Able to build against recent putty version

What was not:
- Windows platform code porting (I've made some modifications, but still insufficient to compile the project, moreover I'm not interested in Windows support)
- MacOS platform code porting (I don't have one to test)

README file in the sources directory is from original qputty and is obsolete.
Icon is based on Breeze (Plasma 5 icon theme) so it is under LGPL3.

Building example:  
git clone https://github.com/dsmorozov/qputty-qt5.git  
git clone git://git.tartarus.org/simon/putty.git  
cd putty  
git checkout tags/0.65  
cd ../qputty-qt5  
qmake  
make  
