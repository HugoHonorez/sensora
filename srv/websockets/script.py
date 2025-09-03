# websocket_server.py
import asyncio
import websockets
import json
import os
from influxdb_client import InfluxDBClient

INFLUX_URL = "http://influxdb:8086"
INFLUX_TOKEN = os.getenv("INFLUX_TOKEN")
ORG = "myorg"
BUCKET = "sensors"
FIELDS = ["temperature", "heatindex", "humidity", "pressure", "light", "airquality"]

client = InfluxDBClient(url=INFLUX_URL, token=INFLUX_TOKEN, org=ORG)
query_api = client.query_api()

def query_data(range_str="-1h", step="30s"):
    flux = f'''
    from(bucket: "{BUCKET}")
      |> range(start: {range_str})
      |> filter(fn: (r) => r["_measurement"] == "mqtt_consumer")
      |> filter(fn: (r) => contains(value: r["_field"], set: {json.dumps(FIELDS)}))
      |> aggregateWindow(every: {step}, fn: mean, createEmpty: false)
      |> yield(name: "mean")
    '''
    result = query_api.query(flux)
    data = []
    for table in result:
        for record in table.records:
            data.append({
                "time": record.get_time().isoformat(),
                "field": record.get_field(),
                "value": record.get_value()
            })
    return data

def query_data_custom(start: str, end: str, step="30s"):
    flux = f'''
    from(bucket: "{BUCKET}")
      |> range(start: time(v: "{start}"), stop: time(v: "{end}"))
      |> filter(fn: (r) => r["_measurement"] == "mqtt_consumer")
      |> filter(fn: (r) => contains(value: r["_field"], set: {json.dumps(FIELDS)}))
      |> aggregateWindow(every: {step}, fn: mean, createEmpty: false)
      |> yield(name: "mean")
    '''
    result = query_api.query(flux)
    data = []
    for table in result:
        for record in table.records:
            data.append({
                "time": record.get_time().isoformat(),
                "field": record.get_field(),
                "value": record.get_value()
            })
    return data

async def handler(websocket):
    print("Client connecté")
    try:
        async for message in websocket:
            print(f"Requête reçue : {message}")
            try:
                params = json.loads(message)
                step = params.get("step", "30s")

                if "custom" in params:
                    start = params["custom"].get("start")
                    end = params["custom"].get("end")
                    data = query_data_custom(start, end, step)
                else:
                    range_str = params.get("range", "-1h")
                    data = query_data(range_str, step)

                await websocket.send(json.dumps({"type": "bulk", "data": data}))
            except Exception as e:
                print("Erreur dans la requête : ", e)
    except websockets.exceptions.ConnectionClosed:
        print("Client déconnecté")

async def main():
    print("WebSocket server running on ws://0.0.0.0:8765")
    async with websockets.serve(handler, "0.0.0.0", 8765):
        await asyncio.Future()

if __name__ == "__main__":
    asyncio.run(main())
