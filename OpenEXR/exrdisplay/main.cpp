///////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2002, Industrial Light & Magic, a division of Lucas
// Digital Ltd. LLC
// 
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
// *       Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
// *       Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
// *       Neither the name of Industrial Light & Magic nor the names of
// its contributors may be used to endorse or promote products derived
// from this software without specific prior written permission. 
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
///////////////////////////////////////////////////////////////////////////


//-----------------------------------------------------------------------------
//
//	exrdisplay -- a simple program to display Imf::Rgba images
//
//-----------------------------------------------------------------------------

#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Value_Slider.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Box.H>

#include <ImageView.h>
#ifdef HAVE_FRAGMENT_SHADERS
#include <ImageViewFragShader.h>
#endif
#include <ImfArray.h>
#include <ImfRgbaFile.h>

#include <iostream>
#include <string>
#include <exception>
#include <string.h>


struct MainWindow
{
    Fl_Window *			window;
    Fl_Box *			exposureLabel;
    Fl_Value_Slider *		exposureSlider;
    Fl_Box *			defogLabel;
    Fl_Value_Slider *		defogSlider;
    Fl_Box *			kneeLowLabel;
    Fl_Value_Slider *		kneeLowSlider;
    Fl_Box *			kneeHighLabel;
    Fl_Value_Slider *		kneeHighSlider;
    ImageView *			image;
    Imf::Array<Imf::Rgba>	pixels;

    static void		exposureSliderCallback (Fl_Widget *widget, void *data);
    static void		defogSliderCallback (Fl_Widget *widget, void *data);
    static void		kneeLowSliderCallback (Fl_Widget *widget, void *data);
    static void		kneeHighSliderCallback (Fl_Widget *widget, void *data);
};


void	
MainWindow::exposureSliderCallback (Fl_Widget *widget, void *data)
{
    MainWindow *mainWindow = (MainWindow *) data;
    mainWindow->image->setExposure (mainWindow->exposureSlider->value());
}


void	
MainWindow::defogSliderCallback (Fl_Widget *widget, void *data)
{
    MainWindow *mainWindow = (MainWindow *) data;
    mainWindow->image->setDefog (mainWindow->defogSlider->value());
}


void	
MainWindow::kneeLowSliderCallback (Fl_Widget *widget, void *data)
{
    MainWindow *mainWindow = (MainWindow *) data;
    mainWindow->image->setKneeLow (mainWindow->kneeLowSlider->value());
}


void	
MainWindow::kneeHighSliderCallback (Fl_Widget *widget, void *data)
{
    MainWindow *mainWindow = (MainWindow *) data;
    mainWindow->image->setKneeHigh (mainWindow->kneeHighSlider->value());
}


MainWindow *
makeMainWindow (const char * imageName, bool useFragmentShader,
		const char * fragmentShaderName)
{
    MainWindow *mainWindow = new MainWindow;

    //
    // Read file imageName.
    //

    Imf::RgbaInputFile in (imageName);

    int w = in.dataWindow().max.x - in.dataWindow().min.x + 1;
    int h = in.dataWindow().max.y - in.dataWindow().min.y + 1;
    int dx = in.dataWindow().min.x;
    int dy = in.dataWindow().min.y;

    mainWindow->pixels.resizeErase (w * h);
    in.setFrameBuffer (mainWindow->pixels - dx - dy * w, 1, w);

    try
    {
	in.readPixels (in.dataWindow().min.y, in.dataWindow().max.y);
    }
    catch (const std::exception &e)
    {
	std::cerr << e.what() << std::endl;
    }

    //
    // Build main window
    //

    Fl::set_color (FL_GRAY, 150, 150, 150);

    mainWindow->window =
	new Fl_Window (w + 10, h + 110, imageName);

    //
    // Add exposure slider
    //

    mainWindow->exposureLabel =
	new Fl_Box (5, 5, 60, 20, "exposure");

    mainWindow->exposureSlider =
	new Fl_Value_Slider (70, 5, w - 65, 20, "");

    enum Fl_When when = useFragmentShader ? FL_WHEN_CHANGED : FL_WHEN_RELEASE;

    mainWindow->exposureSlider->type (FL_HORIZONTAL);
    mainWindow->exposureSlider->range (-10.0, +10.0);
    mainWindow->exposureSlider->step (1, 8);
    mainWindow->exposureSlider->value (0.0);
    mainWindow->exposureSlider->when (when);

    mainWindow->exposureSlider->callback
	(MainWindow::exposureSliderCallback, mainWindow);

    //
    // Add defog slider
    //

    mainWindow->defogLabel =
	new Fl_Box (5, 30, 60, 20, "defog");

    mainWindow->defogSlider =
	new Fl_Value_Slider (70, 30, w - 65, 20, "");

    mainWindow->defogSlider->type (FL_HORIZONTAL);
    mainWindow->defogSlider->range (0.0, 0.01);
    mainWindow->defogSlider->step (1, 10000);
    mainWindow->defogSlider->value (0.0);
    mainWindow->defogSlider->when (when);

    mainWindow->defogSlider->callback
	(MainWindow::defogSliderCallback, mainWindow);

    //
    // Add kneeLow slider
    //

    mainWindow->kneeLowLabel =
	new Fl_Box (5, 55, 60, 20, "knee low");

    mainWindow->kneeLowSlider =
	new Fl_Value_Slider (70, 55, w - 65, 20, "");

    mainWindow->kneeLowSlider->type (FL_HORIZONTAL);
    mainWindow->kneeLowSlider->range (-3.0, 3.0);
    mainWindow->kneeLowSlider->step (1, 8);
    mainWindow->kneeLowSlider->value (0.0);
    mainWindow->kneeLowSlider->when (when);

    mainWindow->kneeLowSlider->callback
	(MainWindow::kneeLowSliderCallback, mainWindow);

    //
    // Add kneeHigh slider
    //

    mainWindow->kneeHighLabel =
	new Fl_Box (5, 80, 60, 20, "knee high");

    mainWindow->kneeHighSlider =
	new Fl_Value_Slider (70, 80, w - 65, 20, "");

    mainWindow->kneeHighSlider->type (FL_HORIZONTAL);
    mainWindow->kneeHighSlider->range (3.5, 7.5);
    mainWindow->kneeHighSlider->step (1, 8);
    mainWindow->kneeHighSlider->value (5.0);
    mainWindow->kneeHighSlider->when (when);

    mainWindow->kneeHighSlider->callback
	(MainWindow::kneeHighSliderCallback, mainWindow);

    //
    // Add image view
    //

#ifdef HAVE_FRAGMENT_SHADERS
    if (useFragmentShader)
    {
	mainWindow->image =
	    new ImageViewFragShader (5, 105, w, h, "",
				     mainWindow->pixels,
				     mainWindow->exposureSlider->value(),
				     mainWindow->defogSlider->value(),
				     mainWindow->kneeLowSlider->value(),
				     mainWindow->kneeHighSlider->value(),
				     fragmentShaderName);
    }
    else
    {
	mainWindow->image =
	    new ImageView (5, 105, w, h, "",
			   mainWindow->pixels,
			   mainWindow->exposureSlider->value(),
			   mainWindow->defogSlider->value(),
			   mainWindow->kneeLowSlider->value(),
			   mainWindow->kneeHighSlider->value());
    }
#else
    mainWindow->image =
	new ImageView (5, 105, w, h, "",
		       mainWindow->pixels,
		       mainWindow->exposureSlider->value(),
		       mainWindow->defogSlider->value(),
		       mainWindow->kneeLowSlider->value(),
		       mainWindow->kneeHighSlider->value());
#endif

    mainWindow->image->box (FL_ENGRAVED_BOX);

    mainWindow->window->end();

    return mainWindow;
}


void
usage (char * pname)
{
    std::cerr << "usage: " << pname << " [-f [fragshader]] imagefile" <<
	std::endl << std::endl;
    std::cerr << "options:" << std::endl << std::endl;
    std::cerr << "    -f [fragshader]    Use a fragment shader to render the image (if supported)." << std::endl;
    std::cerr << "                       If the optional fragshader is specified, use it" << std::endl;
    std::cerr << "                       rather than the built-in fragment shader." << std::endl;
}

int
main(int argc, char **argv)
{
    int exitStatus = 1;
    bool useFragmentShader = false;
    std::string fragmentShaderName;
    std::string imageFileName;

    if (argc < 2 || argc > 4)
    {
	usage (argv[0]);
	return exitStatus;
    }
    
    std::string arg (argv[1]);
    if (arg == "-f")
    {
	useFragmentShader = true;
	if (argc == 4)
	{
	    fragmentShaderName = argv[2];
	    imageFileName = argv[3];
	}
	else if (argc == 3)
	    imageFileName = argv[2];
	else
	{
	    usage (argv[0]);
	    return exitStatus;
	}
    }
    else if (argc > 2)
    {
	usage (argv[0]);
	return exitStatus;
    }
    else
	imageFileName = arg;
    
#ifndef HAVE_FRAGMENT_SHADERS
    if (useFragmentShader)
    {
	std::cerr << argv[0] << " was not compiled with fragment shader support," << std::endl;
	std::cerr << "falling back to software" << std::endl;
	useFragmentShader = false;
	fragmentShaderName.erase ();
    }
#endif

    try
    {
	MainWindow *mainWindow = makeMainWindow (imageFileName.c_str (),
						 useFragmentShader,
						 fragmentShaderName.c_str ());
	mainWindow->window->show (1, argv);
	exitStatus = Fl::run();
    }
    catch (const std::exception &e)
    {
	std::cerr << e.what() << std::endl;
	exitStatus = 1;
    }

    return exitStatus;
}
