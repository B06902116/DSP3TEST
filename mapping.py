import sys
import io
in_name = sys.argv[1]
f_in = io.open(in_name, 'r', encoding='BIG5-HKSCS')
dic = {}
for line in f_in:
	line_sep = line.split(' ')
	ChungWan = line_sep[0]
	ZuYinSet = line_sep[1].split('/')
	flag = 0
	for zuyin in ZuYinSet:
		if zuyin[0] in dic:
			if flag == 0:
				dic[zuyin[0]].append(ChungWan)
				flag = 1
		else:
			dic[zuyin[0]] = []
			dic[zuyin[0]].append(ChungWan)
			flag = 1;
f_in.close()

out_name = sys.argv[2]
f_out = io.open(out_name,'w',encoding='BIG5-HKSCS')
for k in sorted(dic.keys()):#k = bug, pug, mug...
	f_out.write(k + ' '+' '.join(dic[k]) + '\n')
	for chungwan in dic[k]:
		f_out.write(chungwan + ' ' + ' '.join(chungwan) + '\n')
f_out.close()








