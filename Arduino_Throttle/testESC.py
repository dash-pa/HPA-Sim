#!/usr/bin/python3

import pygame
from pygame.locals import *

bgc = (159, 182, 205)

def display(str):
    text = font.render(str, True, (255, 255, 255), (159, 182, 205))
    textRect = text.get_rect()
    textRect.centerx = screen.get_rect().centerx
    textRect.centery = screen.get_rect().centery
    frect = textRect.copy()
    frect.width = screen.get_width()
    frect.left = 0
    screen.fill(bgc, frect)
    screen.blit(text, textRect)
    pygame.display.update()

pygame.init()
screen = pygame.display.set_mode( (640,480) )
pygame.display.set_caption('Python numbers')
screen.fill(bgc)
clk = pygame.time.Clock()

font = pygame.font.Font(None, 17)

while True:
    for event in pygame.event.get():
        if event.type == QUIT:
            pygame.quit()
            exit()
        #elif event.type == KEYDOWN:            
        #elif event.type == KEYUP:
        
    # pygame.event.pump()
    keys = pygame.key.get_pressed()
    s = []
    if keys[K_ESCAPE]:
        s.append('ESC')
    if keys[K_LSHIFT]:
        s.append('SHIFT')
    display( '+'.join(s) )
    clk.tick(100) # 100 fps



'''
import sys, select, tty, termios, time

def isData():
    return select.select([sys.stdin], [], [], 0) == ([sys.stdin], [], [])

old_settings = termios.tcgetattr(sys.stdin)
try:
    tty.setcbreak(sys.stdin.fileno())

    while 1:
        #time.sleep(10)

        if isData():
            c = sys.stdin.read(1)
            if c == '\x1b':         # x1b is ESC
                print('ESC pressed')
                break
            else:
                print(hex(ord(c)))

finally:
    termios.tcsetattr(sys.stdin, termios.TCSADRAIN, old_settings)
print('done')
'''
