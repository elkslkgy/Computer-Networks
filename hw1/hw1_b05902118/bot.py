#coding=utf-8
from bs4 import BeautifulSoup
import socket
import random
import requests
import re
import select
import sys

IRCSocket = socket.socket( socket.AF_INET, socket.SOCK_STREAM )
IRCSocket.connect( ( '140.112.28.111', 6667 ) )

Msg = 'NICK bot_b05902118\r\n'
IRCSocket.send( bytes( Msg , encoding = 'utf-8') )
Msg = 'USER b05902118\r\n'
IRCSocket.send( bytes( Msg , encoding = 'utf-8') )
Msg = 'JOIN #CN_DEMO\r\n'
IRCSocket.send( bytes( Msg , encoding = 'utf-8') )
Msg = "PRIVMSG #CN_DEMO :I'm b05902118\r\n"
IRCSocket.send( bytes( Msg , encoding = 'utf-8') )

star = [':Capricorn', ':Aquarius', ':Pisces', ':Aries', ':Taurus', ':Gemini', \
		':Cancer', ':Leo', ':Virgo', ':Libra', ':Scorpio', ':Sagittarius']

one_to_ten = ['1', '2', '3', '4', '5', '6', '7', '8', '9', '10']

user_guess = []
ques = []
num = []
low = []
up = []

user_chat = []

fd_list =[IRCSocket, sys.stdin]
while True:
	readable, writable, errored = select.select(fd_list, [], [])
	for fd in readable :
		if fd is IRCSocket :
			IRCMsg = IRCSocket.recv(4096).decode()
			#print (IRCMsg)

			temp_Msg = IRCMsg.splitlines()
			cut_Msg = temp_Msg[0].split(' ')
			
			if cut_Msg[1] == "PRIVMSG" and cut_Msg[2] == "bot_b05902118" :
				#bot_name
				cut_point = cut_Msg[0].find('!')
				name = cut_Msg[0][1:cut_point]

				if name in user_guess :	#user is guessing
					place = user_guess.index(name)
					cut_Msg = temp_Msg[0].split(":")

					if cut_Msg[-1] not in one_to_ten :
						continue

					guess = int(cut_Msg[-1])

					if guess == ques[place] :
						Msg = 'PRIVMSG '+ name +" :正確答案為"+ str(guess)+"! 恭喜猜中\r\n"
						IRCSocket.send( bytes( Msg , encoding = 'utf-8') )
						user_guess.remove(name)
						del ques[place]
						del num[place]
						del low[place]
						del up[place]

					elif guess < ques[place] :
							
						if guess < low[place] :
							Msg = 'PRIVMSG '+ name +" :都說大於"+str(low[place])+"了!機靈點！！\r\n"
							IRCSocket.send( bytes( Msg , encoding = 'utf-8') )
							num[place][guess-1] = 1

						elif num[place][guess-1] == 0 :
							Msg = 'PRIVMSG '+ name +" :大於"+cut_Msg[-1]+"!\r\n"
							IRCSocket.send( bytes( Msg , encoding = 'utf-8') )
							num[place][guess-1] = 1
							low[place] = guess

						else :
							Msg = 'PRIVMSG '+ name +" :你猜過"+cut_Msg[-1]+"了(傻眼=_=) 大於"+cut_Msg[-1]+"!\r\n"
							IRCSocket.send( bytes( Msg , encoding = 'utf-8') )

					elif guess > ques[place] :
							
						if guess > up[place] :
							Msg = 'PRIVMSG '+ name +" :都說小於"+str(up[place])+"了!機靈點！！\r\n"
							IRCSocket.send( bytes( Msg , encoding = 'utf-8') )
							num[place][guess-1] = 1

						elif num[place][guess-1] == 0 :
							Msg = 'PRIVMSG '+ name +" :小於"+cut_Msg[-1]+"!\r\n"
							IRCSocket.send( bytes( Msg , encoding = 'utf-8') )
							num[place][guess-1] = 1
							up[place] = guess

						else :
							Msg = 'PRIVMSG '+ name +" :你猜過"+cut_Msg[-1]+"了(傻眼=_=) 小於"+cut_Msg[-1]+"!\r\n"
							IRCSocket.send( bytes( Msg , encoding = 'utf-8') )

				elif name in user_chat :	#user is chatting
					if cut_Msg[3] == ":!bye" :
						print ("========", name, "離開聊天室======== <")
						user_chat.remove(name)
						continue
					s = ' '.join(cut_Msg[3:])
					s = s[1:]
					print ('\r', name, ":", s)
					print (' > ', end = '', flush = True)
					#gogogog
					continue

				elif cut_Msg[3] in star :	#constellation
					Msg = 'PRIVMSG '+ name +" :今天走狗屎運，小心地板黃金\r\n"
					IRCSocket.send( bytes( Msg , encoding = 'utf-8') )

				elif cut_Msg[3] == ":!guess" :	#guess number
					Msg = 'PRIVMSG '+ name +" :猜一個1~10之間的數字！\r\n"
					IRCSocket.send( bytes( Msg , encoding = 'utf-8') )
					user_guess.append(name)
					ques.append(random.randint(1, 10))
					num.append([0, 0, 0, 0, 0, 0, 0, 0, 0, 0])
					low.append(0)
					up.append(10)

				elif cut_Msg[3] == ":!song" :	#web crawler
					song_Msg = temp_Msg[0].split('!')
					song = song_Msg[-1][5:]

					url = "https://www.youtube.com/results?search_query=" + song
					
					resp = requests.get(url)
					soup = BeautifulSoup(resp.text,'html.parser')

					for entry in soup.select('a') :
						m = re.search("v=(.*)", entry['href'])
						if m :
							break

					url = "https://www.youtube.com/watch?v=" + m.group(1)
					Msg = 'PRIVMSG '+ name +" :"+ url +"\r\n"
					IRCSocket.send( bytes( Msg , encoding = 'utf-8') )

				elif cut_Msg[3] == ":!chat" :	#chat
					if len(user_chat) == 0 :
						print (" > ========", name, "想跟你聯繫======== <")
						user_chat.append(name)
					else :
						Msg = 'PRIVMSG '+ name +" :對不起！忙線中，請稍後再試\r\n"
						IRCSocket.send( bytes( Msg , encoding = 'utf-8') )
		elif fd is sys.stdin :
			print (' > ', end = '', flush = True)
			s = sys.stdin.readline()
			Msg = 'PRIVMSG '+ user_chat[0] +" "+ s +"\r\n"
			IRCSocket.send( bytes( Msg , encoding = 'utf-8') )
