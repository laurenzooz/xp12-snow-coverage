#include "XPLMDataAccess.h"
#include "XPLMPlugin.h"
#include "XPLMProcessing.h"
#include "XPLMUtilities.h"

#include <string>
#include <cstring>

#include "httplib.h"
#include <nlohmann/json.hpp>

static float last_depth = 1.25;




float convert_units(float depth_m)
{
	if (depth_m > 0.4) {
		return 0;
	} else if (depth_m > 0.05) {
		return ( (1 / (20 * depth_m)) - 0.2 + 0.2 * depth_m);
	} else if (depth_m > 0) {
		return (-10 * depth_m + 1.2);
	}	
	else {
		return 1.25;
	}
	
}


bool initialized = false;

static XPLMDataRef lat = XPLMFindDataRef("sim/flightmodel/position/latitude");
static XPLMDataRef lon = XPLMFindDataRef("sim/flightmodel/position/longitude");


static float set_depth(float time_since_last_call, float time_since_last_floop, int floop_counter, void* inRefcon) 
{	
	//std::cout << "settin depth\n";
	if (initialized){
	

	

	//if (lat && lon && initialized){
		httplib::Client cli("api.open-meteo.com");
    	auto res = cli.Get("/v1/forecast?latitude=" + std::to_string(XPLMGetDataf(lat)) + "&longitude=" + std::to_string(XPLMGetDataf(lon)) + "&hourly=snow_depth&forecast_days=1");
    	//auto res = cli.Get("/v1/forecast?latitude=62.52&longitude=23.409994&hourly=snow_depth&forecast_days=1");

    	if (res && res->status == 200) {
        	
			nlohmann::json j = nlohmann::json::parse(res->body);



			// get the current hour
			auto now = std::chrono::system_clock::now();
  			std::time_t t_c = std::chrono::system_clock::to_time_t(now);
  			std::tm* utc_tm = std::gmtime(&t_c);
  			int hour = utc_tm->tm_hour;

			if (hour < 0)  { hour = 0; } else if (hour > 23) { hour = 23; }
			//last_depth = translate_units(snow_depth[10]);
			float depth = j["hourly"]["snow_depth"][hour].get<float>();
			
			//last_depth = 0;
			if (!std::isnan(depth)) {
				last_depth = convert_units(depth);


			}
    	} else {
			XPLMDebugString("Snow data not found\n");
		}
	//} 
	}
    return 600;
}



static float floop_cb(float time_since_last_call, float time_since_last_floop, int floop_counter, void* inRefcon) 
{

        //if (initialized)
        //{
		XPLMDataRef snowRef = XPLMFindDataRef("sim/private/controls/wxr/snow_now");
		if (XPLMGetDataf(lat) != 0.0 && XPLMGetDataf(lon) != 0.0) {
			//std::cout << ("snowref is\n");
			if (!initialized) {
				initialized = true;
				set_depth(10, 10, 1, NULL);
				//std::cout << "initialized\n";
			}
			XPLMSetDataf(snowRef, last_depth); 

            
		}
        //}
    return -1;
}

PLUGIN_API int XPluginStart(
    char * outName,
    char * outSig,
    char * outDesc)
{
    strcpy(outName, "SnowCover");
    strcpy(outSig, "laurenzo.snowcover");
    strcpy(outDesc, "A plugin that sets the snow cover when the temp is below 0"); 
	
    
    XPLMRegisterFlightLoopCallback(floop_cb, -1, NULL); // flight loop every cycle
    XPLMRegisterFlightLoopCallback(set_depth, 600, NULL); // update snow depth every 5 mins

	//XPLMDebugString("starting snowcover plugin\n");

    return 1;
}

PLUGIN_API void	XPluginStop(void) 
{ 
	XPLMUnregisterFlightLoopCallback(floop_cb, NULL);
	XPLMUnregisterFlightLoopCallback(set_depth, NULL);
	
}

PLUGIN_API void XPluginDisable(void) { }

PLUGIN_API int XPluginEnable(void) { return 1; }

PLUGIN_API void XPluginReceiveMessage(
    XPLMPluginID	inFromWho,
    int				inMessage,
    void *			inParam)
{

}

