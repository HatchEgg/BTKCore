/* 
 * The Biomechanical ToolKit
 * Copyright (c) 2009-2014, Arnaud Barré
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 *     * Redistributions of source code must retain the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials
 *       provided with the distribution.
 *     * Neither the name(s) of the copyright holders nor the names
 *       of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written
 *       permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "btkMEXObjectHandle.h"
#include "btkMXAnalog.h"

#include <btkAcquisition.h>
#include <btkAnalog.h>

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
  if (nrhs != 3)
    mexErrMsgTxt("Three inputs required.");
  if (nlhs > 2)
    mexErrMsgTxt("Too many output arguments.");

  if ((mxGetClassID(prhs[2]) != mxDOUBLE_CLASS) || mxIsEmpty(prhs[2]) || mxIsComplex(prhs[2]))
    mexErrMsgTxt("The third input must be a vector of real (double) values corresponding to the data of one analog channel."); 

  btk::Acquisition::Pointer acq = btk_MOH_get_object<btk::Acquisition>(prhs[0]);
  btk::Analog::Pointer analog = btkMXGetAnalog(acq, nrhs, prhs);
  int numberOfFrames = analog->GetFrameNumber();
  if (mxGetNumberOfElements(prhs[2]) != numberOfFrames)
    mexErrMsgTxt("The third input doesn't have the same number of element than the number of analog frames.");
    
  double* values = mxGetPr(prhs[2]);
  for (int i = 0 ; i < numberOfFrames ; ++i)
    analog->GetValues().coeffRef(i) = values[i];
  
  // Return updated analog channels
  btkMXCreateAnalogsStructure(acq, nlhs, plhs);
};


