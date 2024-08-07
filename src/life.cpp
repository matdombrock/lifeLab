/*
This is the main entry point

This is responsible for iterating through generations
The actual GOL logic is in `Petri.h`

It also holds logic for generating gifs
*/

#include <vector>
#include <cstdint>
#include <iostream>
#include <math.h>
#include <ctime>
#include "../includes/gif.h"
#include "Matrix.h"
#include "Canvas.h"
#include "Petri.h"
#include "Organism.h"
#include "SorgLoader.h"
#include "CfgLoader.h"
#include "Analysis.h"
#include "util/Colorizer.h"
#include "util/CLIO.h"
#include "util/FileIO.h"

void init(Petri &dish, Cfg cfg, time_t t)
{
    if(cfg.sorg == "noise")
    {
        CLIO::print("Generating noise...");
        dish.randomize(t, 0.5f);
        return;
    }
    SorgLoader::load(dish, "sorgs/"+cfg.sorg+".sorg", cfg.sorgXOff, cfg.sorgYOff, cfg.sorgCenter);
}

void drawFrame1(Canvas &canvas, Cfg cfg)
{
    // Empty first frame
    canvas.clear();
    canvas.draw(0,0);
    canvas.draw(cfg.width-1, 0);
    canvas.draw(0,cfg.height-1);
    canvas.draw(cfg.width-1,cfg.height-1);
}

int main(int argc, char* argv[])
{
    std::string cfgPath = "./cfg.txt";
    if (argc > 1) cfgPath = argv[1];
    Cfg cfg = CfgLoader::load(cfgPath);

    GifWriter g;

    // Get the current timestamp
    std::time_t t = std::time(0);  // t is an integer type

    std::string fileName = cfg.fileName + std::to_string(t);// need to add timestamp
    std::string fileNameGif = "./out/" + fileName + ".gif";
    std::string fileNameAna = "./out/" + fileName + ".html";

    int outputW = cfg.width * cfg.scale;
    int outputH = cfg.height * cfg.scale;
    GifBegin(&g, fileNameGif.c_str(), outputW, outputH, cfg.delay);

    Canvas canvas(cfg.width, cfg.height, cfg.scale);
    
    // Empty first frame
    auto frame = canvas.getBuffer();
    GifWriteFrame(&g, frame.data(), outputW, outputH, cfg.delay);
    
    Petri dish(cfg.width, cfg.height, cfg.ruleSet);

    FinalAnalysis finalAnalysis;

    init(dish, cfg, t);
    
	for(int i = 0; i < cfg.frames + cfg.pre; i++)
	{
        canvas.clear();

        auto dishBuffer = dish.getBuffer();

        // Draw the Petri dish to the buffer
        for (int ii = 0; ii < dishBuffer.size(); ii++)
        {
            if (dishBuffer[ii] > 0)
            {
                std::vector<uint8_t> pixel = Colorizer::colorPixel(dishBuffer[ii], cfg.palette);
                Pixel rgb;
                rgb.r = pixel[0];
                rgb.g = pixel[1];
                rgb.b = pixel[2];
                canvas.draw(ii, rgb);
            }
        }
        GenerationAnalysis analysis = dish.nextGen();
        finalAnalysis.analyze(analysis);

        auto frame = canvas.getBuffer();

        // Delay the first generation
        if(i >= cfg.pre)
        {
            CLIO::print("frame: "+std::to_string(i+1-cfg.pre));
            int delay = (i - cfg.pre) == 0 ? cfg.delay * 4 : cfg.delay;
		    GifWriteFrame(&g, frame.data(), outputW, outputH, delay);
        }
        else
        {
            CLIO::print("pre: "+std::to_string(i+1));
        }
        if (analysis.popDeath){
            // all are dead
            CLIO::print("!!!Population Death!!!");
            break;
        }
        if (analysis.frozen){
            // all are dead
            CLIO::print("!!!Population Frozen!!!");
            break;
        }
	}

    CLIO::print("Finalizing data...");
	GifEnd(&g);
    finalAnalysis.finalize();
    CLIO::print("Saving analysis files...");
    finalAnalysis.save(fileNameAna, fileName, cfg.toString(false));

    CLIO::print("---");
    CLIO::print("DONE!");
    CLIO::print("Saved to:      "+fileNameGif);
    CLIO::print("Analysis at:   "+fileNameAna);
	return 0;
}
