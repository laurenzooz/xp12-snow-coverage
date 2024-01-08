#include "XPLMDataAccess.h"
#include "XPLMPlugin.h"
#include "XPLMProcessing.h"
#include "XPLMUtilities.h"

#include <string>

#include "httplib.h"
#include <nlohmann/json.hpp>

double last_depth = 1.25;




double translate_units(double depth_m)
{
/*
	return 0;
	std::cout << "converitn\n";
	if (depth_m > 0.5) {
		return 0;
	}
	else if (depth_m > 0) {
		return 0.3;
	} else {
		return 1.25;
	}
*/
	std::cout << "converitn\n";
	if (depth_m > 0.5) {
		return 0;
	} else if (depth_m > 0.15) {
		return ( (1 / (10 * depth_m)) - 0.1 - 0.2 * depth_m);
	} else if (depth_m > 0) {
		return (-7.1 * depth_m + 1.3);
	}	
	else {
		return 1.25;
	}
	
}





static float get_depth(float time_since_last_call, float time_since_last_floop, int floop_counter, void* inRefcon) 
{	
	std::cout << "gettin depth\n";

	XPLMDataRef lat = XPLMFindDataRef("sim/flightmodel/position/latitude");
	XPLMDataRef lon = XPLMFindDataRef("sim/flightmodel/position/longitude");

	if (lat){
		httplib::Client cli("api.open-meteo.com");
    	auto res = cli.Get("/v1/forecast?latitude=62.52&longitude=23.409994&hourly=snow_depth&forecast_days=1");
		//std::cout << res;
    	if (res && res->status == 200) {
        	
			auto json = nlohmann::json::parse(res->body);
			auto hourly = json["hourly"];
    		auto snow_depth = hourly["snow_depth"];

/*
			// get the current hour
			auto now = std::chrono::system_clock::now();
  			std::time_t t_c = std::chrono::system_clock::to_time_t(now);
  			std::tm* utc_tm = std::gmtime(&t_c);
  			int hour = utc_tm->tm_hour;
  			std::cout << "The current hour in UTC is: " << hour << std::endl; 

			if (hour < 0)  { hour = 0; } else if (hour >= snow_depth.size()) { hour = snow_depth.size() - 1; }
	*/		//last_depth = translate_units(snow_depth[10]);
			if (snow_depth.size() > 0) { last_depth = translate_units(snow_depth[10]); } else { return 120; }
			//last_depth = 0;
			std::cout << "Snow depth: " << snow_depth[10] << " xp value: " <<  last_depth << std::endl;
    	} else {
			std::cout << "error\n";
		}
	} 
    return 120;
}
bool initialized = false;
static float floop_cb(float time_since_last_call, float time_since_last_floop, int floop_counter, void* inRefcon) 
{
	XPLMDataRef snowRef = XPLMFindDataRef("sim/private/controls/wxr/snow_now");
        if (snowRef)
        {
			if (!initialized) {
				initialized = true;
				get_depth(0, 0, 0, NULL);
				std::cout << "initialized\n";
			}
            XPLMSetDataf(snowRef, last_depth); 
			
        }
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
    XPLMRegisterFlightLoopCallback(get_depth, 120, NULL); // update snow depth every 5 mins

    return 1;
}

PLUGIN_API void	XPluginStop(void) 
{ 
	
}

PLUGIN_API void XPluginDisable(void) { }

PLUGIN_API int XPluginEnable(void) { return 1; }

PLUGIN_API void XPluginReceiveMessage(
    XPLMPluginID	inFromWho,
    int				inMessage,
    void *			inParam)
{
    
}

