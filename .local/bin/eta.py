#!/bin/python3
# This script estimates ETA between origin and destination, and writes to files in /tmp
# example config yaml

"""
dest:
  # home
  lat: 37.7749
  lon: -22.0589
orig:
  # work
  lat: 30.0211
  lon: 34.3363

api_key: "xxx"
"""

import requests
import os
import json
import datetime
import yaml
import argparse
import subprocess
from pytz import timezone

def parse_arguments():
    parser = argparse.ArgumentParser(description="read config.yaml")
    parser.add_argument("file", metavar="CONFIG.yaml", type=str, help="path to the YAML config file")
    return parser.parse_args()

def routes_request(orig, dest, api_key):
    now = datetime.datetime.now(timezone('UTC'))
    future_time = now + datetime.timedelta(minutes=5)
    tnow = future_time.strftime('%Y-%m-%dT%H:%M:%S.%f') + '000Z'

    # URL and headers
    url = 'https://routes.googleapis.com/directions/v2:computeRoutes'
    headers = {
        'Content-Type': 'application/json',
        'X-Goog-Api-Key': api_key,
        'X-Goog-FieldMask': 'routes.duration,routes.distanceMeters,routes.polyline.encodedPolyline'
    }

    # Data payload
    data = {
        "origin": {
            "location": {
                "latLng": {
                    "latitude": orig["lat"],
                    "longitude": orig["lon"]
                }
            }
        },
        "destination": {
            "location": {
                "latLng": {
                    "latitude": dest["lat"],
                    "longitude": dest["lon"]
                }
            }
        },
        "travelMode": "DRIVE",
        "routingPreference": "TRAFFIC_AWARE",
        "departureTime": str(tnow),
        "computeAlternativeRoutes": True,
        "routeModifiers": {
            "avoidTolls": False,
            "avoidHighways": False,
            "avoidFerries": False
        },
        "languageCode": "en-US",
        "units": "IMPERIAL"
    }

    # Sending the request
    response = requests.post(url, headers=headers, data=json.dumps(data))
    # Print the response
    if (response.status_code != 200):
        print("response_status ", response.status_code)
        exit(1)
    return response

def eta2file(orig, dest, file_name, api_key):
    response = routes_request(orig, dest, api_key)
    result = response.json()
    duration = result['routes'][0]['duration'][:-1]
    minutes_duration = round(float(duration)/60,1)

    with open(file_name, "w") as file:
        file.write(str(minutes_duration))

def main():
    args = parse_arguments()

    with open(args.file, 'r') as file:
        data = yaml.safe_load(file)

    print(data["api_key"])
    eta2file(data["orig"], data["dest"], "/tmp/eta2home", data["api_key"])
    eta2file(data["dest"], data["orig"], "/tmp/eta2work", data["api_key"])
    exit(0)

if __name__ == "__main__":
    main()
