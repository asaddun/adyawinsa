[
    {
        "id": "6976075262caf9e1",
        "type": "websocket in",
        "z": "6e29ed9e7ed7f280",
        "name": "Websocket Server",
        "server": "2fa3cc6b3a238722",
        "client": "",
        "x": 110,
        "y": 100,
        "wires": [
            [
                "2809891b66aa65c3",
                "0d5feaa60ee24b1e"
            ]
        ]
    },
    {
        "id": "4bdcb12986078455",
        "type": "http request",
        "z": "6e29ed9e7ed7f280",
        "name": "API",
        "method": "POST",
        "ret": "txt",
        "paytoqs": "ignore",
        "url": "http://apik.adyawins.com/smsd/api/arduino.php",
        "tls": "",
        "persist": false,
        "proxy": "",
        "insecureHTTPParser": false,
        "authType": "",
        "senderr": false,
        "headers": [],
        "x": 570,
        "y": 240,
        "wires": [
            [
                "bac1f18ed6c3fb50",
                "response-handler-node"
            ]
        ]
    },
    {
        "id": "bac1f18ed6c3fb50",
        "type": "debug",
        "z": "6e29ed9e7ed7f280",
        "name": "Response API",
        "active": false,
        "tosidebar": true,
        "console": false,
        "tostatus": false,
        "complete": "payload",
        "targetType": "msg",
        "statusVal": "",
        "statusType": "auto",
        "x": 760,
        "y": 260,
        "wires": []
    },
    {
        "id": "6276c49aa07daa68",
        "type": "json",
        "z": "6e29ed9e7ed7f280",
        "name": "",
        "property": "payload",
        "action": "",
        "pretty": false,
        "x": 430,
        "y": 240,
        "wires": [
            [
                "4bdcb12986078455"
            ]
        ]
    },
    {
        "id": "3fdf8ba30820dcaa",
        "type": "trigger",
        "z": "6e29ed9e7ed7f280",
        "name": "",
        "op1": "",
        "op2": "",
        "op1type": "nul",
        "op2type": "payl",
        "duration": "10",
        "extend": false,
        "overrideDelay": false,
        "units": "s",
        "reset": "",
        "bytopic": "all",
        "topic": "topic",
        "outputs": 1,
        "x": 270,
        "y": 240,
        "wires": [
            [
                "6276c49aa07daa68"
            ]
        ]
    },
    {
        "id": "2809891b66aa65c3",
        "type": "function",
        "z": "6e29ed9e7ed7f280",
        "name": "parse action",
        "func": "var data = JSON.parse(msg.payload);\nvar obj = data.action;\nvar a;\nvar b;\nif (obj == \"temp\") {\n    a = { payload: data };\n} else if (obj == \"shoot\") {\n    b = { payload: data };\n}\nreturn [a, b];",
        "outputs": 2,
        "noerr": 0,
        "initialize": "",
        "finalize": "",
        "libs": [],
        "x": 130,
        "y": 160,
        "wires": [
            [
                "9408059bfc92ba7f"
            ],
            [
                "8be9048583e1035d"
            ]
        ]
    },
    {
        "id": "9408059bfc92ba7f",
        "type": "function",
        "z": "6e29ed9e7ed7f280",
        "name": "split by id",
        "func": "var data = msg.payload;\nvar obj = data.id;\nvar a;\nvar b;\nvar c;\nif (obj == \"1000059\") {\n    a = { payload: data };\n} else if (obj == \"1000078\") {\n    b = { payload: data };\n} else if (obj == \"1000002\") {\n    c = { payload: data };\n}\nreturn [a, b, c];",
        "outputs": 3,
        "noerr": 0,
        "initialize": "",
        "finalize": "",
        "libs": [],
        "x": 120,
        "y": 240,
        "wires": [
            [
                "3fdf8ba30820dcaa"
            ],
            [],
            []
        ]
    },
    {
        "id": "396f3d91be2ee3b2",
        "type": "debug",
        "z": "6e29ed9e7ed7f280",
        "name": "debug 2",
        "active": false,
        "tosidebar": true,
        "console": false,
        "tostatus": false,
        "complete": "payload",
        "targetType": "msg",
        "statusVal": "",
        "statusType": "auto",
        "x": 440,
        "y": 300,
        "wires": []
    },
    {
        "id": "8be9048583e1035d",
        "type": "function",
        "z": "6e29ed9e7ed7f280",
        "name": "parseJSON",
        "func": "const data = msg.payload;\nconst action = data.action;\nconst id = data.id;\nconst inj = { payload: data.inj};\nconst cycletime = data.cyc;\nconst dt = data.dt;\nconst time = data.time;\n\nconst ip = data.ip;\n\nmsg.payload = {\n    action,\n    id,\n    cycletime,\n    dt,\n    ip,\n    time\n};\nreturn [inj, msg];",
        "outputs": 2,
        "noerr": 0,
        "initialize": "",
        "finalize": "",
        "libs": [],
        "x": 110,
        "y": 300,
        "wires": [
            [
                "a8d49224bcce06a0"
            ],
            [
                "a8d49224bcce06a0"
            ]
        ]
    },
    {
        "id": "a8d49224bcce06a0",
        "type": "trigger",
        "z": "6e29ed9e7ed7f280",
        "name": "",
        "op1": "",
        "op2": "0",
        "op1type": "pay",
        "op2type": "str",
        "duration": "0",
        "extend": false,
        "overrideDelay": false,
        "units": "ms",
        "reset": "1",
        "bytopic": "all",
        "topic": "topic",
        "outputs": 1,
        "x": 280,
        "y": 300,
        "wires": [
            [
                "396f3d91be2ee3b2",
                "6276c49aa07daa68"
            ]
        ]
    },
    {
        "id": "27b0d31c7fa1edb0",
        "type": "inject",
        "z": "6e29ed9e7ed7f280",
        "name": "",
        "props": [
            {
                "p": "payload"
            }
        ],
        "repeat": "",
        "crontab": "",
        "once": false,
        "onceDelay": 0.1,
        "topic": "",
        "payload": "{\"action\":\"shoot\",\"id\":1000,\"cla\":1,\"inj\":1,\"cycletime\":8,\"dt\":0,\"ip\":\"192.168.10.96\",\"time\":\"20:00:00\"}",
        "payloadType": "json",
        "x": 430,
        "y": 200,
        "wires": [
            [
                "4bdcb12986078455"
            ]
        ]
    },
    {
        "id": "response-handler-node",
        "type": "function",
        "z": "6e29ed9e7ed7f280",
        "name": "Response Status",
        "func": "if (msg.statusCode >= 200 && msg.statusCode < 300) {\n    node.status({ fill: \"green\", shape: \"dot\", text: \"Success\" });\n    return msg;\n} else {\n    node.status({ fill: \"red\", shape: \"dot\", text: \"Failed\" });\n}",
        "outputs": 1,
        "noerr": 0,
        "initialize": "",
        "finalize": "",
        "libs": [],
        "x": 770,
        "y": 200,
        "wires": [
            [
                "b1c8cd1555dd95c9",
                "1e2d0c1220fe54b4"
            ]
        ]
    },
    {
        "id": "b1c8cd1555dd95c9",
        "type": "file in",
        "z": "6e29ed9e7ed7f280",
        "name": "Read single.txt",
        "filename": "/testnodered/single.txt",
        "filenameType": "str",
        "format": "lines",
        "chunk": false,
        "sendError": false,
        "encoding": "none",
        "allProps": false,
        "x": 960,
        "y": 240,
        "wires": [
            [
                "220ec51debe90fb5"
            ]
        ]
    },
    {
        "id": "42a855d02fe64a06",
        "type": "function",
        "z": "6e29ed9e7ed7f280",
        "name": "Collecting from log.txt",
        "func": "var newData = msg.payload;\nvar tempData = flow.get('temp') || [];\ntempData.push(newData);\nflow.set('temp', tempData);\n// var tempData = \"[\";\n// tempData += newData;\n// tempData += \"]\";\n// msg.payload = tempData;\n\nreturn msg;",
        "outputs": 1,
        "noerr": 0,
        "initialize": "",
        "finalize": "",
        "libs": [],
        "x": 1460,
        "y": 240,
        "wires": [
            [
                "1e81dceac38e9d1d"
            ]
        ]
    },
    {
        "id": "7f62316a85b5b180",
        "type": "debug",
        "z": "6e29ed9e7ed7f280",
        "name": "Batch sent",
        "active": true,
        "tosidebar": true,
        "console": false,
        "tostatus": false,
        "complete": "payload",
        "targetType": "msg",
        "statusVal": "",
        "statusType": "auto",
        "x": 1550,
        "y": 280,
        "wires": []
    },
    {
        "id": "ac4a402cf365415b",
        "type": "catch",
        "z": "6e29ed9e7ed7f280",
        "name": "catch single json",
        "scope": [
            "4bdcb12986078455"
        ],
        "uncaught": false,
        "x": 760,
        "y": 120,
        "wires": [
            [
                "1f3e9c6f9325703c"
            ]
        ]
    },
    {
        "id": "de47da071d79b0d7",
        "type": "file",
        "z": "6e29ed9e7ed7f280",
        "name": "Write single.txt",
        "filename": "/testnodered/single.txt",
        "filenameType": "str",
        "appendNewline": true,
        "createDir": false,
        "overwriteFile": "false",
        "encoding": "none",
        "x": 1120,
        "y": 120,
        "wires": [
            [
                "3e906ec5ed4bcecf"
            ]
        ]
    },
    {
        "id": "3e906ec5ed4bcecf",
        "type": "debug",
        "z": "6e29ed9e7ed7f280",
        "name": "Write to file",
        "active": true,
        "tosidebar": true,
        "console": false,
        "tostatus": false,
        "complete": "payload",
        "targetType": "msg",
        "statusVal": "",
        "statusType": "auto",
        "x": 1210,
        "y": 160,
        "wires": []
    },
    {
        "id": "918e4f5cb1083d33",
        "type": "file",
        "z": "6e29ed9e7ed7f280",
        "name": "Clear single.txt",
        "filename": "/testnodered/single.txt",
        "filenameType": "str",
        "appendNewline": false,
        "createDir": false,
        "overwriteFile": "true",
        "encoding": "none",
        "x": 1320,
        "y": 360,
        "wires": [
            []
        ]
    },
    {
        "id": "df79ef6f98073f17",
        "type": "inject",
        "z": "6e29ed9e7ed7f280",
        "name": "",
        "props": [
            {
                "p": "payload"
            }
        ],
        "repeat": "",
        "crontab": "",
        "once": false,
        "onceDelay": 0.1,
        "topic": "",
        "payload": "",
        "payloadType": "date",
        "x": 760,
        "y": 300,
        "wires": [
            [
                "1e2d0c1220fe54b4",
                "b1c8cd1555dd95c9"
            ]
        ]
    },
    {
        "id": "a61a41f0e72e6c61",
        "type": "function",
        "z": "6e29ed9e7ed7f280",
        "name": "Send flow.temp",
        "func": "var tempData = flow.get('temp');\nmsg.payload = tempData;\nreturn msg;",
        "outputs": 1,
        "noerr": 0,
        "initialize": "",
        "finalize": "",
        "libs": [],
        "x": 1140,
        "y": 280,
        "wires": [
            [
                "f796922c4b2ad25c",
                "26fa0e443efc36d3"
            ]
        ]
    },
    {
        "id": "1e81dceac38e9d1d",
        "type": "trigger",
        "z": "6e29ed9e7ed7f280",
        "name": "",
        "op1": "",
        "op2": "",
        "op1type": "nul",
        "op2type": "date",
        "duration": "250",
        "extend": false,
        "overrideDelay": false,
        "units": "ms",
        "reset": "",
        "bytopic": "all",
        "topic": "topic",
        "outputs": 1,
        "x": 960,
        "y": 280,
        "wires": [
            [
                "a61a41f0e72e6c61",
                "710b5bf38f91011f"
            ]
        ]
    },
    {
        "id": "710b5bf38f91011f",
        "type": "function",
        "z": "6e29ed9e7ed7f280",
        "name": "Clear file",
        "func": "msg.payload = \"\";\n\nreturn msg;",
        "outputs": 1,
        "noerr": 0,
        "initialize": "",
        "finalize": "",
        "libs": [],
        "x": 1120,
        "y": 320,
        "wires": [
            [
                "918e4f5cb1083d33",
                "2eccc89aad2458ac"
            ]
        ]
    },
    {
        "id": "f796922c4b2ad25c",
        "type": "function",
        "z": "6e29ed9e7ed7f280",
        "name": "Clear flow.temp",
        "func": "flow.set('temp', undefined);\nmsg.payload = \"flow.temp has been cleared\";\nreturn msg;\n",
        "outputs": 1,
        "noerr": 0,
        "initialize": "",
        "finalize": "",
        "libs": [],
        "x": 1320,
        "y": 320,
        "wires": [
            [
                "6fceb94f7ead867c"
            ]
        ]
    },
    {
        "id": "6fceb94f7ead867c",
        "type": "debug",
        "z": "6e29ed9e7ed7f280",
        "name": "Clear",
        "active": false,
        "tosidebar": true,
        "console": false,
        "tostatus": false,
        "complete": "payload",
        "targetType": "msg",
        "statusVal": "",
        "statusType": "auto",
        "x": 1470,
        "y": 320,
        "wires": []
    },
    {
        "id": "b1a86f1eabdd2beb",
        "type": "http request",
        "z": "6e29ed9e7ed7f280",
        "d": true,
        "name": "API",
        "method": "POST",
        "ret": "txt",
        "paytoqs": "ignore",
        "url": "http://apik.adyawinsa.com/smsd/api/arduino.php",
        "tls": "",
        "persist": false,
        "proxy": "",
        "insecureHTTPParser": false,
        "authType": "",
        "senderr": false,
        "headers": [],
        "x": 1410,
        "y": 200,
        "wires": [
            [
                "7f62316a85b5b180"
            ]
        ]
    },
    {
        "id": "220ec51debe90fb5",
        "type": "function",
        "z": "6e29ed9e7ed7f280",
        "name": "Check single.txt",
        "func": "var i = msg.payload;\nif (i === \"\"){\n} else{\n    return msg;\n}",
        "outputs": 1,
        "noerr": 0,
        "initialize": "",
        "finalize": "",
        "libs": [],
        "x": 1140,
        "y": 240,
        "wires": [
            [
                "57d5eb7df3c6ac0c"
            ]
        ]
    },
    {
        "id": "e1753d76f8bdd55d",
        "type": "inject",
        "z": "6e29ed9e7ed7f280",
        "name": "",
        "props": [
            {
                "p": "payload"
            }
        ],
        "repeat": "",
        "crontab": "",
        "once": false,
        "onceDelay": 0.1,
        "topic": "",
        "payload": "",
        "payloadType": "date",
        "x": 960,
        "y": 320,
        "wires": [
            [
                "710b5bf38f91011f"
            ]
        ]
    },
    {
        "id": "57d5eb7df3c6ac0c",
        "type": "json",
        "z": "6e29ed9e7ed7f280",
        "name": "",
        "property": "payload",
        "action": "",
        "pretty": false,
        "x": 1290,
        "y": 240,
        "wires": [
            [
                "42a855d02fe64a06"
            ]
        ]
    },
    {
        "id": "26fa0e443efc36d3",
        "type": "json",
        "z": "6e29ed9e7ed7f280",
        "name": "",
        "property": "payload",
        "action": "",
        "pretty": false,
        "x": 1290,
        "y": 280,
        "wires": [
            [
                "b1a86f1eabdd2beb",
                "7f62316a85b5b180",
                "88898ddf91d5088e"
            ]
        ]
    },
    {
        "id": "335732d5140b32ff",
        "type": "function",
        "z": "6e29ed9e7ed7f280",
        "name": "Check array or not",
        "func": "if (typeof msg.payload === \"object\") {\n    if (Array.isArray(msg.payload)) {\n        // it's an array\n        return msg;\n    } else {\n        // it's a single JSON object\n    }\n}",
        "outputs": 1,
        "noerr": 0,
        "initialize": "",
        "finalize": "",
        "libs": [],
        "x": 450,
        "y": 100,
        "wires": [
            [
                "feedc8c5c597f031"
            ]
        ]
    },
    {
        "id": "a0c632539f2a1838",
        "type": "inject",
        "z": "6e29ed9e7ed7f280",
        "name": "",
        "props": [
            {
                "p": "payload"
            }
        ],
        "repeat": "",
        "crontab": "",
        "once": false,
        "onceDelay": 0.1,
        "topic": "",
        "payload": "[{\"sec\":165,\"time\":\"08:48:48\"},{\"sec\":5,\"time\":\"08:48:53\"},{\"sec\":7,\"time\":\"08:49:01\"},{\"sec\":0,\"time\":\"08:49:01\"}]",
        "payloadType": "json",
        "x": 430,
        "y": 60,
        "wires": [
            [
                "335732d5140b32ff"
            ]
        ]
    },
    {
        "id": "b868679a1975b8fb",
        "type": "debug",
        "z": "6e29ed9e7ed7f280",
        "name": "debug 9",
        "active": false,
        "tosidebar": true,
        "console": false,
        "tostatus": false,
        "complete": "payload",
        "targetType": "msg",
        "statusVal": "",
        "statusType": "auto",
        "x": 740,
        "y": 80,
        "wires": []
    },
    {
        "id": "0d5feaa60ee24b1e",
        "type": "json",
        "z": "6e29ed9e7ed7f280",
        "name": "",
        "property": "payload",
        "action": "",
        "pretty": false,
        "x": 290,
        "y": 100,
        "wires": [
            [
                "335732d5140b32ff"
            ]
        ]
    },
    {
        "id": "feedc8c5c597f031",
        "type": "json",
        "z": "6e29ed9e7ed7f280",
        "name": "",
        "property": "payload",
        "action": "",
        "pretty": false,
        "x": 610,
        "y": 100,
        "wires": [
            [
                "b868679a1975b8fb",
                "ef612f1a41efda8c"
            ]
        ]
    },
    {
        "id": "1f3e9c6f9325703c",
        "type": "function",
        "z": "6e29ed9e7ed7f280",
        "name": "parseJSON",
        "func": "const data = msg.payload;\nconst action = data.action;\nconst id = data.id;\nconst inj = { payload: data.inj};\nconst cycletime = data.cycletime;\nconst dt = data.dt;\nconst time = data.time;\n\nconst ip = data.ip;\n\nmsg.payload = {\n    id,\n    cycletime,\n    ip,\n    time\n};\nreturn msg;",
        "outputs": 1,
        "noerr": 0,
        "initialize": "",
        "finalize": "",
        "libs": [],
        "x": 950,
        "y": 120,
        "wires": [
            [
                "de47da071d79b0d7"
            ]
        ]
    },
    {
        "id": "1f888bccea1ae378",
        "type": "file",
        "z": "6e29ed9e7ed7f280",
        "name": "Write batch.txt",
        "filename": "/testnodered/batch.txt",
        "filenameType": "str",
        "appendNewline": true,
        "createDir": false,
        "overwriteFile": "false",
        "encoding": "none",
        "x": 960,
        "y": 160,
        "wires": [
            [
                "3e906ec5ed4bcecf"
            ]
        ]
    },
    {
        "id": "2eccc89aad2458ac",
        "type": "file",
        "z": "6e29ed9e7ed7f280",
        "name": "Clear batch.txt",
        "filename": "/testnodered/batch.txt",
        "filenameType": "str",
        "appendNewline": false,
        "createDir": false,
        "overwriteFile": "true",
        "encoding": "none",
        "x": 1320,
        "y": 400,
        "wires": [
            []
        ]
    },
    {
        "id": "ef612f1a41efda8c",
        "type": "http request",
        "z": "6e29ed9e7ed7f280",
        "name": "API",
        "method": "POST",
        "ret": "txt",
        "paytoqs": "ignore",
        "url": "http://apik.adyawins.com/smsd/api/arduino.php",
        "tls": "",
        "persist": false,
        "proxy": "",
        "insecureHTTPParser": false,
        "authType": "",
        "senderr": false,
        "headers": [],
        "credentials": {},
        "x": 570,
        "y": 200,
        "wires": [
            [
                "response-handler-node",
                "bac1f18ed6c3fb50"
            ]
        ]
    },
    {
        "id": "34f8c23c0d960d07",
        "type": "catch",
        "z": "6e29ed9e7ed7f280",
        "name": "catch array json",
        "scope": [
            "ef612f1a41efda8c"
        ],
        "uncaught": false,
        "x": 760,
        "y": 160,
        "wires": [
            [
                "1f888bccea1ae378"
            ]
        ]
    },
    {
        "id": "1e2d0c1220fe54b4",
        "type": "file in",
        "z": "6e29ed9e7ed7f280",
        "name": "Read batch.txt",
        "filename": "/testnodered/batch.txt",
        "filenameType": "str",
        "format": "lines",
        "chunk": false,
        "sendError": false,
        "encoding": "none",
        "allProps": false,
        "x": 960,
        "y": 200,
        "wires": [
            [
                "0176f1200262420a"
            ]
        ]
    },
    {
        "id": "0176f1200262420a",
        "type": "function",
        "z": "6e29ed9e7ed7f280",
        "name": "Check batch.txt",
        "func": "var i = msg.payload;\nif (i === \"\"){\n} else{\n    return msg;\n}",
        "outputs": 1,
        "noerr": 0,
        "initialize": "",
        "finalize": "",
        "libs": [],
        "x": 1140,
        "y": 200,
        "wires": [
            [
                "b1a86f1eabdd2beb",
                "7f62316a85b5b180",
                "710b5bf38f91011f"
            ]
        ]
    },
    {
        "id": "88898ddf91d5088e",
        "type": "file",
        "z": "6e29ed9e7ed7f280",
        "d": true,
        "name": "Write single.txt",
        "filename": "/testnodered/test.txt",
        "filenameType": "str",
        "appendNewline": true,
        "createDir": true,
        "overwriteFile": "false",
        "encoding": "none",
        "x": 1660,
        "y": 320,
        "wires": [
            []
        ]
    },
    {
        "id": "2fa3cc6b3a238722",
        "type": "websocket-listener",
        "path": "/",
        "wholemsg": "false"
    }
]
