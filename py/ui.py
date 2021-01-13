import tkinter as tk
import asyncio
import websockets
import threading
import json
import queue


uri = "ws://localhost:49999"

# interface for UI, used by SynthProxy
class UI(tk.Tk):
    def __init__(self):
        super().__init__()
        self.create_widgets()

    def create_widgets(self):
        pass

    def render_osc(self):
        pass

    def get_osc_sliders(self) -> []:
        pass

    def get_ad_sliders(self) -> []:
        pass

# SynthProxy as webservice based interface for the Synth running elsewhere
class SynthProxy:
    def init(self, ui):
        self.ui = ui
        self.queue = queue.Queue()
        self.running=True

    async def run(self):   
        while self.running:
            async with websockets.connect(uri) as websocket:
                self.ws = websocket
                
                while self.running:
                    rpc = self.queue.get()
                    await self.ws.send(json.dumps(rpc))
                    res = await self.ws.recv()
                    await self.updateUI()
                    self.queue.task_done()

    def stop(self):
        self.running=False
        self.ws.close();

    async def updateUI(self):
        rpc = { "rpc": "render-osc", "arg": { "length": 1024 }}
        await self.ws.send(json.dumps(rpc))
        res = await self.ws.recv()
        resData = json.loads(res)
        ui.render_osc(resData)

    def updateOsc(self, v):
        sliders = self.ui.get_osc_sliders()
        rpc = { "rpc": "set-osc-parameters", 
                 "arg": [ { "pos": sliders[0].get(), "target": sliders[1].get() }, 
                          { "pos": sliders[2].get(), "target": sliders[3].get() }, 
                          { "pos": sliders[4].get(), "target": sliders[5].get() }, 
                          { "pos": sliders[6].get(), "target": sliders[7].get() }, 
                          { "pos": sliders[8].get(), "target": sliders[9].get() }, 
                          { "pos": sliders[10].get(), "target": sliders[11].get() }, 
                          { "pos": sliders[12].get(), "target": sliders[13].get() }, 
                          { "pos": sliders[14].get(), "target": sliders[15].get() }]}
        
        if not self.queue.full():
            self.queue.put_nowait(rpc)
        else:
            print("queue is full")

    def updateAD(self, v):
        sliders = self.ui.get_ad_sliders()
        rpc = { "rpc": "set-env-parameters", 
                 "arg": { "attack": sliders[0].get(), "decay": sliders[1].get() }}
        
        if not self.queue.full():
            self.queue.put_nowait(rpc)
        else:
            print("queue is full")

# implementation of the actual UI
class TkUI(UI):
    def init(self, synth):
        self.synth = synth

    def create_widgets(self):
        self.title("sine segment osc")
        self.geometry('1024x800')

        self.canvas = tk.Canvas(self, bg="white")
        self.canvas.place(x=0, y=0, width=1024, height=500)

        self.osc_sliders = []
        self.ad_sliders = []
    
        elementWidth = 1024 / 8

        for i in range(0, 8):
            target = tk.Scale(self, from_=1, to=-1, resolution=0.01, orient='vertical', command=synth.updateOsc)
            target.place(x=i * elementWidth, y=500, width=elementWidth, height=100)
            pos = tk.Scale(self, from_=0, to=1, resolution=0.01, orient='horizontal', command=synth.updateOsc)
            pos.place(x=i * elementWidth, y=600, width=elementWidth, height=100)
            pos.set(i * 1 / 8)
            self.osc_sliders.append(pos)
            self.osc_sliders.append(target)

    
        a = tk.Scale(self, from_=0, to=2000, resolution=0.01, orient='horizontal', command=synth.updateAD)
        a.place(x=0, y=700, width=200, height=50)
        d = tk.Scale(self, from_=0, to=2000, resolution=0.01, orient='horizontal', command=synth.updateAD)
        d.place(x=400, y=700, width=200, height=50)
        self.ad_sliders.append(a)
        self.ad_sliders.append(d)    

    def render_osc(self, resData):
        self.canvas.delete(tk.ALL)
        lastY = -100
        x = 0

        for i in resData['result']:
            y = 250 - 240 * i
            
            if lastY != -100:
                self.canvas.create_line(x, lastY, x + 1, y, fill="#ff0000")
            
            lastY = y
            x = x + 1

    def get_osc_sliders(self) -> []:
        return self.osc_sliders

    def get_ad_sliders(self) -> []:
        return self.ad_sliders

    def run(self):
        self.mainloop()

synth = SynthProxy()
ui = TkUI()

synth.init(ui)
ui.init(synth)

synth.updateOsc(0)
synth.updateAD(0)

def _asyncio_thread(async_loop):
    async_loop.run_until_complete(synth.run())

async_loop = asyncio.get_event_loop()
threading.Thread(target=_asyncio_thread, args=(async_loop,)).start()
ui.run()
synth.stop()