#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <map>
#include <string>
#include <android/looper.h>
#include <android/sensor.h>
const char kPackageName[] = "com.torgtek.mati_dsms";
const int kLooperId = 1;
int sensor_ret ( float sense[]) {

    ASensorManager* sensor_manager =
            ASensorManager_getInstanceForPackage(kPackageName);
    if (!sensor_manager) {
        fprintf(stderr, "Failed to get a sensor manager\n");
        return 1;
    }
    ASensorList sensor_list;


    ASensorEventQueue* queue = ASensorManager_createEventQueue(
            sensor_manager,
            ALooper_prepare(ALOOPER_PREPARE_ALLOW_NON_CALLBACKS),
            kLooperId, NULL /* no callback */, NULL /* no data */);
    if (!queue) {
        fprintf(stderr, "Failed to create a sensor event queue\n");
        return 1;
    }
    const std::map<int, std::string> kSensorSamples = {
            /*
             * Accelerometer:
             *   Reporting mode: continuous. Events are generated continuously.
             */
            { ASENSOR_TYPE_ACCELEROMETER, "accelerometer" },
            /*
             * Proximity sensor:
             *   Reporting mode: on-change. Events are generated only when proximity
             *     value has changed.
             */
            { ASENSOR_TYPE_PROXIMITY, "proximity sensor" },
            /*
             * Significant motion sensor:
             *   Reporting mode: one-shot. An event is generated when significant
             *     motion is detected. After that, the sensor will be disabled by
             *     itself.
             */
            { ASENSOR_TYPE_SIGNIFICANT_MOTION, "significant motion sensor" },

            { ASENSOR_TYPE_GRAVITY, "gravity" },
    };
    const int kNumSamples = 1;
    const int kNumEvents = 1;
    const int kTimeoutMilliSecs = 50;
    const int kWaitTimeSecs = 1;
    for (auto& sensor_type : kSensorSamples) {
        const ASensor* sensor = ASensorManager_getDefaultSensor(sensor_manager,
                                                                sensor_type.first);
        if (sensor && !ASensorEventQueue_enableSensor(queue, sensor)) {
            for (int i = 0; i < kNumSamples; i++) {
                int ident = ALooper_pollAll(kTimeoutMilliSecs,
                                            NULL /* no output file descriptor */,
                                            NULL /* no output event */,
                                            NULL /* no output data */);
                if (ident == kLooperId) {
                    ASensorEvent data;
                    if (ASensorEventQueue_getEvents(queue, &data, kNumEvents)) {
                        if (sensor_type.first == ASENSOR_TYPE_ACCELEROMETER) {
                            printf("Acceleration: x = %f, y = %f, z = %f\n",
                                   data.acceleration.x, data.acceleration.y,
                                   data.acceleration.z);
                            sense[0]=data.acceleration.x;
                            sense[1]=data.acceleration.y;
                            sense[2]=data.acceleration.z;
                        } else if (sensor_type.first == ASENSOR_TYPE_GRAVITY) {
                            printf("Acceleration: x = %f, y = %f, z = %f\n",
                                   data.acceleration.x, data.acceleration.y,
                                   data.acceleration.z);
                            sense[5]=data.acceleration.x;
                            sense[6]=data.acceleration.y;
                            sense[7]=data.acceleration.z;
                        }
                        else if (sensor_type.first == ASENSOR_TYPE_PROXIMITY) {
                            printf("Proximity distance: %f\n", data.distance);
                            sense[3]=data.distance;
                        } else if (sensor_type.first == ASENSOR_TYPE_SIGNIFICANT_MOTION) {
                            if (data.data[0] == 1) {
                                printf("Significant motion detected\n");
                                sense[4]=1.0;
                                break;
                            }else{
                                sense[4]=0.0;
                            }
                        }
                    }
                }

            }
            int ret = ASensorEventQueue_disableSensor(queue, sensor);
            if (ret) {
                fprintf(stderr, "Failed to disable %s: %s\n",
                        sensor_type.second.c_str(), strerror(ret));
            }
        } else {
            fprintf(stderr, "No %s found or failed to enable it\n",
                    sensor_type.second.c_str());
        }
    }
    int ret = ASensorManager_destroyEventQueue(sensor_manager, queue);
    if (ret) {
        fprintf(stderr, "Failed to destroy event queue: %s\n", strerror(ret));
        return 1;
    }
    return 0;
}