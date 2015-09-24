

                             Minimal BVH Send Server
                             =======================
					
                                      v0.1.0
                           
                          Created by Heiko Fink aka Cyx
                                           

                           
Contents:
---------
        I.    Description
        II.   Installation
        III.  Configuration
        IV.   Contact, Donation
        V.    LICENSE


I. Description
==============
This minimal BVH Send server reads in a BVH motion file, opens a listening TCP socket and
sends each motion line to the connected clients.
The delay between the motion lines can be configured or is read from the BVH file.
If all motion lines have been sent the server loops back to the first one.


II. Installation
================
Just download the archive under: https://github.com/Cyx69/bvhsend/releases
and unpack the file to a folder you want.


III. Configuration
==================
Following command line options are available:

bvhsend <port> <frametime> <format> <bvhfile>

For e.g. bvhsend 7001 10000 0 test.bvh

With:
-----
port     : TCP port number.
frametime: Delay between each motion line in microseconds.
           Set to 0 if frame time from BVH file should be used.
format:    0 = Just send complete line as it is in BVH file.
           1 = Use Axis Neuron format.
bvhfile  : Name and path of BVH file to be sent.


IV. Contact, Donation
=====================
Heiko Fink aka Cyx
EMail: hfinkdeletemenospam@web.de

If you find the code or parts of it useful, please support me and
the further development and make a donation here:
https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=DYZEA9A3MLA5Y


V. LICENSE
==========
The MIT License (MIT)

Copyright (c) 2015 Heiko Fink

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE
