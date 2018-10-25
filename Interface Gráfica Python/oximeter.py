from  tkinter import *
import socket

target_host = '127.0.0.1'
target_port = 44445
data1 = 0
data2 = 0

client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

client.connect((target_host, target_port))
print("Connected to {:s}:{:d}".format(target_host, target_port))

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
    global data1, data2
    client.send(str.encode("".join([str(data1)])))  # send some data
    data1 = client.recv(4096)                       # receive some data
    print(data1.decode('utf-8'))
    client.send(str.encode("".join([str(data2)])))  # send some data
    data2 = client.recv(4096)                       # receive some data
    print(data2.decode('utf-8'))

    bpm = Label(root, text=data1, bg="white", fg="black")
    bpm.place(relx=0.65, rely=0.5, anchor=CENTER)
    spo2 = Label(root, text=data2, bg="white", fg="black")
    spo2.place(relx=0.65, rely=0.7, anchor=CENTER)

button1 = Button(text="BPM", width=6)
button1.bind("<Button-1>", printBPM)
button1.place(relx=0.35, rely=0.5, anchor=CENTER)

button2 = Button(text="spO2", width=6)
button2.bind("<Button-1>", printBPM)
button2.place(relx=0.35, rely=0.7, anchor=CENTER)

credits = Label(root, text="Autor: Henrique Jordão Figueiredo Alves", bg="black", fg="white")
credits.pack(side=BOTTOM)

root.mainloop()