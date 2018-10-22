from  tkinter import *
import random #apenas para testar a captura de dados

root = Tk()
root.geometry("480x360")
root.title("Cliente OCF")
root.resizable(0,0)

background = PhotoImage(file="heart.png")
background_label = Label(root, image=background)
background_label.place(x=0, y=0, relwidth=1, relheight=1)

photo = PhotoImage(file="OCF.png")
img = Label(root, image=photo)
img.place(relx=0.5, rely=0.2, anchor=CENTER)

def printBPM(event):
    n = random.randint(85, 120)
    bpm = Label(root, text=n, bg="white", fg="black")
    bpm.place(relx=0.65, rely=0.5, anchor=CENTER)

def printSPO2(event):
    n = random.randint(90, 99)
    spo2 = Label(root, text=n, bg="white", fg="black")
    spo2.place(relx=0.65, rely=0.7, anchor=CENTER)

button1 = Button(text="BPM", width=6)
button1.bind("<Button-1>", printBPM)
button1.place(relx=0.35, rely=0.5, anchor=CENTER)

button2 = Button(text="spO2", width=6)
button2.bind("<Button-1>", printSPO2)
button2.place(relx=0.35, rely=0.7, anchor=CENTER)

credits = Label(root, text="Autor: Henrique Jordão Figueiredo Alves", bg="black", fg="white")
credits.pack(side=BOTTOM)

root.mainloop()