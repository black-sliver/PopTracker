#!/usr/bin/python3

import asyncio
import websockets
import json

connected = set()
variables = [
    { "cmd": "Var", "name": "a", "value": False },
    { "cmd": "Var", "name": "b", "value": 0 },
    { "cmd": "Var", "name": "c", "value": [False,0] }
]

async def handler(websocket, path):
    """minimalistic uat server"""
    connected.add(websocket)
    try:
        info = {
            "cmd": "Info",
            "protocol": 0,
            "name": "Example",
            "version": "1.00"
        }
        await websocket.send(json.dumps([info]))
        async for data in websocket:
            packet = json.loads(data)
            print(f"< {json.dumps(packet)}")
            for command in packet:
                if command["cmd"] == "Sync":
                    data = json.dumps(variables)
                    print(f"> {data}")
                    await websocket.send(data)
    finally:
        connected.remove(websocket)

async def main():
    """toggle/change variables every second one by one"""
    n = 0
    while True:
        await asyncio.sleep(1)
        if n == 0:
            variables[0]["value"] = not variables[0]["value"]
        elif n == 1:
            variables[1]["value"] = (variables[1]["value"]+1)%100
        elif n == 2:
            variables[2]["value"][1] += 1
            if variables[2]["value"][1] == 2:
                variables[2]["value"][0] = not variables[2]["value"][0]
                variables[2]["value"][1] = 0
        for client in connected:
            data = json.dumps([variables[n]])
            print(f"> {data}")
            await client.send(data)
        n = (n+1)%3

start_server = websockets.serve(handler, "localhost", 65399)
loop = asyncio.get_event_loop()
loop.run_until_complete(start_server)
loop.create_task(main())
loop.run_forever()
