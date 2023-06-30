import subprocess
import pandas as pd

TESTCASE_DIR = './testcases/'

TESTCASE_NAMES = [
    'array_test1', 'array_test2', 'basicopt1', 'bulgarian', 'expr', 'gcd',
    'hanoi', 'lvalue2', 'magic', 'manyarguments', 'multiarray', 'naive',
    'pi', 'qsort', 'queens', 'statement_test', 'superloop', 'tak'
]

ALU_NUM = [2, 4]

PREDICTORS = [
    'twobit', 'local', 'gshare', 'tournament'
]

def construct_cmd(test_name, alun, pred_name):
    return ' '.join([
        '.\\code.exe',
        TESTCASE_DIR + test_name + '.data',
        '-A', str(alun),
        '-P', pred_name, '10',
        '-stat'
    ])

names = []
preds = []
alus = []
result = []
cycles = []
p00 = []
p01 = []
p10 = []
p11 = []

for name in TESTCASE_NAMES:
    for alun in ALU_NUM:
        for pred in PREDICTORS:
            cmd = construct_cmd(name, alun, pred)
            print('running:', cmd)
            p = subprocess.run(cmd, shell= True, stdout= subprocess.PIPE)
            s = p.stdout.decode('gbk')
            ls = s.split('\n')
            names.append(name)
            preds.append(pred)
            alus.append(alun)
            result.append(int(ls[0]))
            cycles.append(int(ls[1]))
            ll = ls[2].split('\t')
            p00.append(int(ll[0]))
            p01.append(int(ll[1]))
            ll = ls[3].split('\t')
            p10.append(int(ll[0]))
            p11.append(int(ll[1]))

df = pd.DataFrame(
    {
        'name': names,
        'predictor': preds,
        'alun': alus,
        'result': result,
        'cycle': cycles,
        'P00': p00,
        'P01': p01,
        'P10': p10,
        'P11': p11,
    }
)

df.to_csv('./illustration/result.csv')
