import tkinter as tk
from tkinter import filedialog
import asyncio
import websockets
import threading
import json
import queue
import numpy as np
from functools import partial
import sys
import os

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
    
    def get_synth_sliders(self) -> []:
        pass

    def get_morph_slider(self) -> []:
        pass
    

# SynthProxy as webservice based interface for the Synth running elsewhere
class SynthProxy:
    def init(self, ui):
        self.ui = ui
        self.queue = queue.Queue()
        self.running=True
        self.oldtarget0 = 0

    async def run(self):
        while self.running:
            async with websockets.connect(uri) as websocket:
                self.ws = websocket
                
                while self.running:
                    await self.sendRPC()
                    
                    while not self.queue.empty():
                        await self.sendRPC()
                        
                    await self.updateUI()
                    
    async def sendRPC(self):
        rpc = self.queue.get()
        await self.ws.send(json.dumps(rpc))
        res = await self.ws.recv()
        self.queue.task_done()

    def stop(self):
        self.running=False
        self.ws.close()

    async def updateUI(self):
        rpc = { "rpc": "render-osc", "arg": { "length": 1024 }}
        await self.ws.send(json.dumps(rpc))
        res = await self.ws.recv()
        resData = json.loads(res)
        ui.render_osc(resData)

    def updateOsc(self, v):
        sliders = self.ui.get_osc_sliders()
        simplify = self.ui.simplify
        autark = self.ui.autark

        # saw/square crossfader
        if (simplify == 1):
            if(sliders[16].get()>=0.5):
                sliders[9].set((sliders[16].get()-0.5)*2)
                sliders[10].set((sliders[16].get()-0.5)*2)
                sliders[15].set((sliders[16].get()-0.5)*2)
                sliders[14].set((sliders[16].get()-0.5)*2)
            else:
                sliders[9].set(0)
                sliders[10].set(abs(1-sliders[16].get()*2))
                sliders[14].set(0)
                sliders[15].set(abs(1-sliders[16].get()*2))


        if (autark == 1):

            #pairing Amp-Slider
            sliders[1].set(sliders[0].get())
            sliders[3].set(sliders[2].get())
            sliders[5].set(sliders[4].get())
            sliders[7].set(sliders[6].get())
        
        pos4 = sliders[12].get()

        pos2 = pos4 * sliders[11].get()

        pos1 = pos2 * sliders[9].get()
        
        pos3 = pos2 + (pos4 - pos2) * sliders[10].get()
        
        pos6 = pos4 + (1 - pos4) * sliders[13].get()
            
        pos5 = pos4 + (pos6 - pos4) * sliders[14].get()
                
        pos7 = pos6 + (1 - pos6) * sliders[15].get()

        if (sliders[0].get() != self.oldtarget0):
            sliders[8].set(sliders[0].get())
            self.oldtarget0 = sliders[0].get()
            
        if (sliders[8].get() != self.oldtarget0):
            sliders[0].set(sliders[8].get())
            self.oldtarget0 = sliders[8].get()
       
        rpc = { "rpc": "set-osc-parameters", 
                 "arg": [ { "pos": 0,    "target": sliders[1].get() }, 
                          { "pos": pos1, "target": sliders[2].get() }, 
                          { "pos": pos2, "target": sliders[3].get() }, 
                          { "pos": pos3, "target": sliders[4].get() }, 
                          { "pos": pos4, "target": sliders[5].get() }, 
                          { "pos": pos5, "target": sliders[6].get() }, 
                          { "pos": pos6, "target": sliders[7].get() }, 
                          { "pos": pos7, "target": sliders[0].get() }]}
        
        if not self.queue.full():
            self.queue.put_nowait(rpc)
        else:
            print("queue is full")

    def updateAD(self, v):
        sliders = self.ui.get_ad_sliders()
        rpc = { "rpc": "set-env-parameters", 
                 "arg": { "attack": 10**(sliders[0].get()/10), "decay": 10**(sliders[1].get()/10)}}
        self.ui.label_attack.config(text=f"{10**(sliders[0].get()/10):.2f}")
        self.ui.label_release.config(text=f"{10**(sliders[1].get()/10):.2f}")
        
        if not self.queue.full():
            self.queue.put_nowait(rpc)
        else:
            print("queue is full")
            
    def updateSynth(self, v):
        sliders = self.ui.get_synth_sliders()
        rpc = { "rpc": "set-synth-parameters", 
                 "arg": { "master": sliders[0].get() }}
        
        if not self.queue.full():
            self.queue.put_nowait(rpc)
        else:
            print("queue is full")


# implementation of the actual UI
class TkUI(UI):
    def init(self, synth):
        self.synth = synth
        self.version = os.path.basename(sys.argv[0])

    def create_widgets(self):
        self.title("sine segment osc")
        self.geometry('1920x1080')

        self.canvas = tk.Canvas(self, bg="white")
        self.canvas.place(x=0, y=0, width=1024, height=500)

        self.osc_sliders = []
        self.ad_sliders = []
        self.synth_sliders = []
        self.morph_slider = []
        self.simplify = 1
        self.autark = 0

        # 19 is hardcoded
        self.savedValuesOsc = np.zeros((4, 17))
        self.currentValues = []
        elementWidth = 133  # 1024 / 8


        # Amplitude Sliders
        for i in range(0, 9):
            target = tk.Scale(self, from_=1, to=-1, resolution=0.01, orient='vertical', command=synth.updateOsc)     
            target.place(x=5 + i*117, y=505, width=60, height=128)
            self.osc_sliders.append(target)

        # Shift Sliders
        for i in range(0, 7):
            pos = tk.Scale(self, from_=0.00001, to=0.99999, resolution=0.00001, orient='horizontal', command=synth.updateOsc)
            self.osc_sliders.append(pos)

        # intesity slider
        intensity = tk.Scale(self, from_=0.00001, to=0.99999, resolution=0.00001, orient='horizontal', command=synth.updateOsc)
        intensity.set(0.5)
        self.osc_sliders.append(intensity)
        self.osc_sliders[16].place(x=148, y=630, width=740, height=40)

        self.LetAmpSlidersDisappear()
        
        # Simplify Button
        self.simplify_button = tk.Button(self, text="SIMPLIFY", command=self.simplifyIt)
        self.simplify_button.place(x=20, y=945, width=70, height=30)

        # Autark Button
        self.autark_button = tk.Button(self, text="AUTARK", command=self.autarkIt)
        self.autark_button.place(x=100, y=945, width=70, height=30)

        # Default Button
        self.default = tk.Button(self, text="DEFAULT", command=self.makeDefaults)
        self.default.place(x=1080, y=0, width=150, height=40)

        # Save Button 1
        saveValues1 = partial(self.SaveValues, 1)
        self.save1 = tk.Button(self, text="Save in A", command=saveValues1)
        self.save1.place(x=1080, y=40, width=150, height=40)

        # Save Button 2
        saveValues2 = partial(self.SaveValues, 2)
        self.save2 = tk.Button(self, text="Save in B", command=saveValues2)
        self.save2.place(x=1080, y=80, width=150, height=40)

        # Save Button 3
        saveValues3 = partial(self.SaveValues, 3)
        self.save3 = tk.Button(self, text="Save in C", command=saveValues3)
        self.save3.place(x=1080, y=120, width=150, height=40)

        # Save Button 4
        saveValues4 = partial(self.SaveValues, 4)
        self.save4 = tk.Button(self, text="Save in D", command=saveValues4)
        self.save4.place(x=1080, y=160, width=150, height=40)

        # A Button 
        restoreValues1 = partial(self.RestoreValues, 1)
        self.restore1 = tk.Button(self, text="A", command=restoreValues1)
        self.restore1.place(x=1080, y=400, width=40, height=40)

        # B Button 
        restoreValues2 = partial(self.RestoreValues, 2)
        self.restore2 = tk.Button(self, text="B", command=restoreValues2)
        self.restore2.place(x=1547, y=400, width=40, height=40)

        # C Button 
        restoreValues3 = partial(self.RestoreValues, 3)
        self.restore3 = tk.Button(self, text="C", command=restoreValues3)
        self.restore3.place(x=1080, y=830, width=40, height=40)

        # D Button 
        restoreValues4 = partial(self.RestoreValues, 4)
        self.restore4 = tk.Button(self, text="D", command=restoreValues4)
        self.restore4.place(x=1547, y=830, width=40, height=40)

        # ActivatePad
        self.activate = tk.Scale(self, from_=0, to=1, resolution=1, orient='vertical')
        self.activate.place(x=1547, y=615, width=100, height=60)
            
        # Presets
        Sine = partial(self.makePreset, 1)
        self.sine = tk.Button(self, text="Sine", command=Sine)
        self.sine.place(x=1300, y=0, width=150, height=40)

        Saw = partial(self.makePreset, 2)
        self.sine = tk.Button(self, text="Saw", command=Saw)
        self.sine.place(x=1300, y=40, width=150, height=40)

        Pulse = partial(self.makePreset, 3)
        self.sine = tk.Button(self, text="Pulse", command=Pulse)
        self.sine.place(x=1300, y=80, width=150, height=40)

        Stairs = partial(self.makePreset, 4)
        self.sine = tk.Button(self, text="Stairs", command=Stairs)
        self.sine.place(x=1300, y=120, width=150, height=40)
        

        # Scale Sliders 25 percent
        # self.osc_sliders[9].place(x=92, y=630, width=150, height=40)
        # self.osc_sliders[10].place(x=92 + 2 * 117, y=630, width=150, height=40)
        # self.osc_sliders[14].place(x=92 + 4 * 117, y=630, width=150, height=40)
        # self.osc_sliders[15].place(x=92 + 6 * 117, y=630, width=150, height=40)
        
        # 50 percent
        self.osc_sliders[11].place(x=148, y=670, width=270, height=40)           
        self.osc_sliders[13].place(x=618, y=670, width=270, height=40)           

        # 100 percent
        self.osc_sliders[12].place(x=262, y=710, width=510, height=40)           


        #attack
        self.label_attack = tk.Label()
        self.label_attack.pack()
        self.label_attack.place(x=54, y=751)
        a = tk.Scale(self, from_=0, to=43.0103, resolution=0.01, orient='horizontal', command=synth.updateAD, showvalue=0)
        a.place(x=54, y=770, width=300, height=50)
                
        #release
        self.label_release = tk.Label()
        self.label_release.pack()
        self.label_release.place(x=380, y=751)
        r = tk.Scale(self, from_=0, to=43.0103, resolution=0.01, orient='horizontal', command=synth.updateAD, showvalue=0)
        r.place(x=380, y=770, width=300, height=50)
        

        # Volume
        mainVol = tk.Scale(self, from_=0, to=1, resolution=0.01, orient='horizontal', command=synth.updateSynth)
        mainVol.place(x=710, y=750, width=300, height=50)

        # Save Load Dialog Boxes
        self.savepreset = tk.Button(self, text="Save Preset", command=Presets.WritePreset)
        self.savepreset.place(x=1580, y=0, width=100, height=40)

        self.loadpreset = tk.Button(self, text="Load Preset", command=Presets.ReadPreset)
        self.loadpreset.place(x=1580, y=40, width=100, height=40)

        self.savecomb = tk.Button(self, text="Save Combination", command=Presets.WriteCombination)
        self.savecomb.place(x=1700, y=0, width=150, height=40)

        self.loadcomb = tk.Button(self, text="Load Combination", command=Presets.ReadCombination)
        self.loadcomb.place(x=1700, y=40, width=150, height=40)


        self.ad_sliders.append(a)
        self.ad_sliders.append(r)
        self.synth_sliders.append(mainVol)

        
##        # Morph Slider 1
##        pos = tk.Scale(self, from_=0, to=1, resolution=0.01, orient='horizontal', command=self.MorphAB)
##        pos.place(x= 1080, y=450, width=510, height=40)
##        self.morph_slider.append(pos)
##
##        # Morph Slider 2
##        pos = tk.Scale(self, from_=1, to=0, resolution=0.01, orient='vertical', command=self.MorphABC)
##        pos.place(x=1280.5 , y=500, width=80, height=310)
##        pos.set(1)
##        self.morph_slider.append(pos)

        # Morph Pad
        self.pad = tk.Canvas(self, bg='blue')
        self.pad.place(x=1125, y=445, width=400, height=400)
        self.pad.bind('<Motion>', self.MorphPad)
        self.pad.bind('<Button-1>', lambda event: self.activate.set(1))
        self.pad.bind('<Button-3>', lambda event: self.activate.set(0))

        # Bind (per Schleife funktioniert das mit lambda nicht)
        self.osc_sliders[0].bind('<Button-3>', lambda event: self.osc_sliders[0].set(0))
        self.osc_sliders[1].bind('<Button-3>', lambda event: self.osc_sliders[1].set(0))
        self.osc_sliders[2].bind('<Button-3>', lambda event: self.osc_sliders[2].set(0))
        self.osc_sliders[3].bind('<Button-3>', lambda event: self.osc_sliders[3].set(0))
        self.osc_sliders[4].bind('<Button-3>', lambda event: self.osc_sliders[4].set(0))
        self.osc_sliders[5].bind('<Button-3>', lambda event: self.osc_sliders[5].set(0))
        self.osc_sliders[6].bind('<Button-3>', lambda event: self.osc_sliders[6].set(0))
        self.osc_sliders[7].bind('<Button-3>', lambda event: self.osc_sliders[7].set(0))
        self.osc_sliders[8].bind('<Button-3>', lambda event: self.osc_sliders[8].set(0))
        self.osc_sliders[9].bind('<Button-3>', lambda event: self.osc_sliders[9].set(0.5))
        self.osc_sliders[10].bind('<Button-3>', lambda event: self.osc_sliders[10].set(0.5))
        self.osc_sliders[11].bind('<Button-3>', lambda event: self.osc_sliders[11].set(0.5))
        self.osc_sliders[12].bind('<Button-3>', lambda event: self.osc_sliders[12].set(0.5))
        self.osc_sliders[13].bind('<Button-3>', lambda event: self.osc_sliders[13].set(0.5))
        self.osc_sliders[14].bind('<Button-3>', lambda event: self.osc_sliders[14].set(0.5))
        self.osc_sliders[15].bind('<Button-3>', lambda event: self.osc_sliders[15].set(0.5))
        self.osc_sliders[16].bind('<Button-3>', lambda event: self.osc_sliders[16].set(0.5))

    # this function calls the functions makeAmpSliders and simplifier
    def simplifyIt(self):
        self.simplifier()
        self.LetShiftSlidersDiasappear()
    
    def autarkIt(self):
        self.autarkify()
        self.LetAmpSlidersDisappear()

    def LetAmpSlidersDisappear(self):
        
        if(self.autark == 1):
            for i in range(0, 9):
                if(i % 2 != 0):        
                    self.osc_sliders[i].place(x=0, y=0, width=0, height=0)

        else:
            for i in range(0, 9):
                self.osc_sliders[i].place(x=5 + i*117, y=505, width=60, height=128)


    def LetShiftSlidersDiasappear(self):
        if(self.simplify==1):
            self.osc_sliders[9].place(x=0, y=0, width=0, height=0)
            self.osc_sliders[10].place(x=0, y=0, width=0, height=0)
            self.osc_sliders[14].place(x=0, y=0, width=0, height=0)
            self.osc_sliders[15].place(x=0, y=0, width=0, height=0)
            self.osc_sliders[16].place(x=148, y=630, width=740, height=40)
        else:
            self.osc_sliders[9].place(x=92, y=630, width=150, height=40)
            self.osc_sliders[10].place(x=92 + 2 * 117, y=630, width=150, height=40)
            self.osc_sliders[14].place(x=92 + 4 * 117, y=630, width=150, height=40)
            self.osc_sliders[15].place(x=92 + 6 * 117, y=630, width=150, height=40)
            self.osc_sliders[16].place(x=0, y=0, width=0, height=0) 

    def simplifier(self):
        if(self.simplify==0):
            self.simplify = 1
            return
        if(self.simplify==1):
            self.simplify = 0
            return

    def autarkify(self):
        if(self.autark==0):
            self.autark = 1
            return
        if(self.autark==1):
            self.autark = 0
            return

    def makeDefaults(self):

        #ranges are hardcoded
        for i in range (9):
            self.osc_sliders[i].set(0)
            for i in range(11, 14):
                self.osc_sliders[i].set(0.5)
        for i in range(2):
            self.ad_sliders[i].set(5)

        self.osc_sliders[16].set(0.5)
        self.synth_sliders[0].set(0.4)

    # fuer die ABCD Punkte
    def SaveValues(self, a):
        
        if(a==1):
            for i in range(len(self.osc_sliders)):
                self.savedValuesOsc[0][i] =  self.osc_sliders[i].get()
        if(a==2):
            for i in range(len(self.osc_sliders)):
                self.savedValuesOsc[1][i] =  self.osc_sliders[i].get()
        if(a==3):
            for i in range(len(self.osc_sliders)):
                self.savedValuesOsc[2][i] = self.osc_sliders[i].get()
        if(a==4):
            for i in range(len(self.osc_sliders)):
                self.savedValuesOsc[3][i] = self.osc_sliders[i].get()        
    # fuer die ABCD Punkte
    def RestoreValues(self, a):

        if(a==1):
            for i in range(len(self.osc_sliders)):
                self.osc_sliders[i].set(self.savedValuesOsc[0][i])
##            self.morph_slider[0].set(0)
        if(a==2):
            for i in range(len(self.osc_sliders)):
                self.osc_sliders[i].set(self.savedValuesOsc[1][i])
##            self.morph_slider[0].set(1)
        if(a==3):
            for i in range(len(self.osc_sliders)):
                self.osc_sliders[i].set(self.savedValuesOsc[2][i])
##            self.morph_slider[1].set(0)
        if(a==4):
            for i in range(len(self.osc_sliders)):
                self.osc_sliders[i].set(self.savedValuesOsc[3][i])
##            self.morph_slider[1].set(0)


# not used at the moment 
##    def MorphAB(self, pos):
##        pos = float(pos)
##        for i in range(len(self.osc_sliders)):
##            self.savedValuesOscAB[i] = self.osc_sliders[i].get()
##        for i in range(len(self.osc_sliders)):
##            self.osc_sliders[i].set(pos*self.savedValuesOsc[1][i]+(1-pos)*self.savedValuesOsc[0][i])
##


    
    def MorphPad(self, event):
        if(self.activate.get()==1):
            x = event.x/400
            y = event.y/400
            for i in range(len(self.osc_sliders)):
                self.osc_sliders[i].set(self.savedValuesOsc[0][i]*(1-x)*(1-y)+self.savedValuesOsc[1][i]*x*(1-y)+self.savedValuesOsc[2][i]*(1-x)*y+self.savedValuesOsc[3][i]*x*y)

    #Schnellaufrufpresets
    def makePreset(self, a):
        #Sine
        if(a==1):
            self.osc_sliders[0].set(1)
            self.osc_sliders[2].set(-1)
            self.osc_sliders[4].set(1)
            self.osc_sliders[6].set(-1)
            for i in range(11, 14):
                self.osc_sliders[i].set(0.5)
            self.osc_sliders[16].set(0.5)
        
        #Saw
        if(a==2):
            self.osc_sliders[0].set(1)
            self.osc_sliders[2].set(-1)
            self.osc_sliders[4].set(1)
            self.osc_sliders[6].set(-1)
            self.osc_sliders[11].set(0.00001)
            self.osc_sliders[12].set(0.5)
            self.osc_sliders[13].set(0.00001)
            self.osc_sliders[16].set(0.5)

        #Pulse
        if(a==3):
            self.osc_sliders[0].set(-1)
            self.osc_sliders[2].set(-1)
            self.osc_sliders[4].set(1)
            self.osc_sliders[6].set(-1)

            self.osc_sliders[11].set(0.95)
            self.osc_sliders[12].set(0.5)
            self.osc_sliders[13].set(0.05)
            self.osc_sliders[16].set(0.75)
        
        #Stairs
        if(a==4):
            self.osc_sliders[0].set(-1)
            self.osc_sliders[2].set(-0.33)
            self.osc_sliders[4].set(0.33)
            self.osc_sliders[6].set(1)    
            for i in range(11, 14):
                self.osc_sliders[i].set(0.5)
            self.osc_sliders[16].set(0.99999)    

    # a function that can store the current values
    def StoreCurrentValues(self):
        self.currentValues = []
        for i in range(len(self.osc_sliders)):
            self.currentValues.append(self.osc_sliders[i].get())
        for j in range(len(self.ad_sliders)):
            self.currentValues.append(self.ad_sliders[j].get())
        for k in range(len(self.synth_sliders)):
            self.currentValues.append(self.synth_sliders[k].get())
        return self.currentValues

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
    
    def get_synth_sliders(self) -> []:
        return self.synth_sliders
    def get_morph_slider(self) -> []:
        return self.morph_slider

    def run(self):
        self.mainloop()


class Presets:
    def init(self, ui):
        self.ui = ui
        
    # save preset as csv + versionsuffix
    def WritePreset():
        osc_values = ui.get_osc_sliders()
        ad_values = ui.get_ad_sliders()
        synthvalues = ui.get_synth_sliders()
        all_values = []
        
        for i in range(len(osc_values)):
            all_values.append(osc_values[i].get())
        for j in range(len(ad_values)):
            all_values.append(ad_values[j].get())
        for k in range(len(synthvalues)):
            all_values.append(synthvalues[k].get())
        np.asarray(all_values)
        save_dialog = filedialog.asksaveasfilename()
        save_name = save_dialog + '_' + os.path.basename(sys.argv[0])[:-3] + '.csv'
        np.savetxt(save_name, all_values, delimiter=',')
        


    def ReadPreset():
        osc_values = ui.get_osc_sliders()
        ad_values = ui.get_ad_sliders()
        synthvalues = ui.get_synth_sliders()
        load_dialog = filedialog.askopenfilename()
        loaded_values = np.genfromtxt(load_dialog, delimiter=',')

        j=0
        for i in range(len(osc_values+ad_values+synthvalues)):
            if(i<len(osc_values)):
                osc_values[i].set(loaded_values[i])
            if(i>=len(osc_values) and i<len(osc_values+ad_values)):
                ad_values[j].set(loaded_values[i])
                j+=1
            if(i>=len(osc_values+ad_values) and i<len(osc_values+ad_values+synthvalues)):
                synthvalues[0].set(loaded_values[i])
    
    # save combination as csv + versionsuffix
    def WriteCombination():
        all_values_comb = []
        currentValues = ui.StoreCurrentValues()

        for i in range(len(ui.savedValuesOsc)):
            for j in range(len(ui.savedValuesOsc[0])):
                all_values_comb.append(ui.savedValuesOsc[i][j])
        all_values_comb.extend(currentValues)
        np.asarray(all_values_comb)
        save_dialog = filedialog.asksaveasfilename()
        save_name = save_dialog + '_' + os.path.basename(sys.argv[0])[:-3] + '.csv'
        np.savetxt(save_name, all_values_comb, delimiter=',')

    def ReadCombination():
        osc_values = ui.get_osc_sliders()
        ad_values = ui.get_ad_sliders()
        synthvalues = ui.get_synth_sliders()

        load_dialog = filedialog.askopenfilename()
        loaded_values = np.genfromtxt(load_dialog, delimiter=',')

        k = 0
        for i in range(len(ui.savedValuesOsc)):
            for j in range(len(ui.savedValuesOsc[0])):
                ui.savedValuesOsc[i][j] = loaded_values[k]
                k+=1
        j = 0
        for i in range(len(osc_values+ad_values+synthvalues)):
            if(i<len(osc_values)):
                osc_values[i].set(loaded_values[k])
                k+=1
            if(i>=len(osc_values) and i<len(osc_values+ad_values)):
                ad_values[j].set(loaded_values[k])
                j+=1
                k+=1
            if(i>=len(osc_values+ad_values) and i<len(osc_values+ad_values+synthvalues)):
                synthvalues[0].set(loaded_values[k])
                k+=1


    
#Instantiation
synth = SynthProxy()
ui = TkUI()
presets = Presets()
#Initialization
synth.init(ui)
ui.makeDefaults()
ui.SaveValues(1)
ui.SaveValues(2)
ui.SaveValues(3)
ui.SaveValues(4)
ui.init(synth)
presets.init(ui)


#start update methods
synth.updateOsc(0)
synth.updateAD(0)

def _asyncio_thread(async_loop):
    async_loop.run_until_complete(synth.run())


async_loop = asyncio.get_event_loop()
threading.Thread(target=_asyncio_thread, args=(async_loop,)).start()
ui.run()
synth.stop()
