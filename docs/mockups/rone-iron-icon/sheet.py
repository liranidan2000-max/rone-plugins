from PIL import Image, ImageDraw, ImageFont
R = '../../../RonePluginsCenter/Resources/'
F = ImageFont.truetype('C:/Windows/Fonts/arialbd.ttf', 26); Fs = ImageFont.truetype('C:/Windows/Fonts/arial.ttf', 20)
he = lambda s: s[::-1]
opts = [('NOW', Image.open('../../../RoneIron/Resources/AppIcon.png'), 'היום - הקו האלכסוני'),
        ('A', Image.open('option-A.png'), 'א - גל של קול חתוך לצ\'ופים'),
        ('B', Image.open('option-B.png'), 'ב - מיקרופון חתוך'),
        ('C', Image.open('option-C.png'), 'ג - קו קול ניאון חתוך')]
fam = ['RoneStutter', 'RoneFlanger', 'RoneHook', None, 'RoneGrain', 'RoneStucker']
W = 1400; cell = 330
H = 60 + cell + 110 + len(opts)*170 + 40
s = Image.new('RGB', (W, H), (14, 14, 18)); d = ImageDraw.Draw(s)
d.text((W-40, 20), he('אייקון חדש ל-RONE Iron'.replace('RONE Iron', 'norI ENOR')), font=F, fill=(240, 240, 245), anchor='ra')
for i, (k, im, lab) in enumerate(opts):
    x = 40 + i*cell
    big = im.convert('RGBA').resize((256, 256), Image.LANCZOS); s.paste(big, (x+20, 70), big)
    for j, z in enumerate((64, 32, 20)):
        sm = im.convert('RGBA').resize((z, z), Image.LANCZOS); s.paste(sm, (x+20+j*80, 340+(64-z)//2), sm)
    d.text((x+276, 420), he(lab), font=Fs, fill=(220, 160, 255) if k != 'NOW' else (150, 150, 160), anchor='ra')
y = 480
for k, im, lab in opts:
    d.text((W-40, y+60), he(lab.split(' - ')[0] if k != 'NOW' else 'היום'), font=F, fill=(200, 200, 210), anchor='ra')
    for j, f in enumerate(fam):
        ic = (im if f is None else Image.open(R + f + '_icon.png')).convert('RGBA').resize((140, 140), Image.LANCZOS)
        s.paste(ic, (40 + j*160, y), ic)
    y += 170
s.save('rone-iron-icon-options.png')
