from RLTest import Env

def test_sdset():
    env = Env()
    env.expect('rdict.sdset', 'myDict', '0.1', '10').equal(1)
    env.expect('hgetall', '{myDict}_sd1').equal(['0.1', '10'])
    env.expect('zrange', '{myDict}_sd2', '0', '-1').equal(['0.1'])

def test_sdset_same():
    env = Env()
    env.expect('rdict.sdset', 'myDict', '0.1', '10').equal(1)
    env.expect('rdict.sdset', 'myDict', '0.1', '10').equal(0)
    env.expect('rdict.sdset', 'myDict', '0.1', '20').equal(0)
    env.expect('hgetall', '{myDict}_sd1').equal(['0.1', '20'])
    env.expect('zrange', '{myDict}_sd2', '0', '-1').equal(['0.1'])

def test_sdset_many():
    env = Env()
    env.cmd('rdict.sdset', 'myDict', '0.15', '10')
    env.cmd('rdict.sdset', 'myDict', '0.05', '13')
    env.cmd('rdict.sdset', 'myDict', '0.2', '20')
    env.expect('zrange', '{myDict}_sd2', '0', '-1').equal(['0.05', '0.15', '0.2'])
    env.expect('hlen', '{myDict}_sd1').equal(3)
    env.expect('hget', '{myDict}_sd1', '0.15').equal('10')
    env.expect('hget', '{myDict}_sd1', '0.05').equal('13')
    env.expect('hget', '{myDict}_sd1', '0.2').equal('20')

def test_sdmset():
    env = Env()
    env.expect('rdict.sdmset', 'myDict', '0.15', '10', '0.05', '13', '0.2', '20', '0.05', '12').equal(3)
    env.expect('zrange', '{myDict}_sd2', '0', '-1').equal(['0.05', '0.15', '0.2'])
    env.expect('hlen', '{myDict}_sd1').equal(3)
    env.expect('hget', '{myDict}_sd1', '0.15').equal('10')
    env.expect('hget', '{myDict}_sd1', '0.05').equal('12')
    env.expect('hget', '{myDict}_sd1', '0.2').equal('20')

def test_sdget():
    env = Env()
    env.cmd('rdict.sdset', 'myDict', '1', '10')
    env.cmd('rdict.sdset', 'myDict', '0.5', 'x')
    env.expect('rdict.sdget', 'myDict', '1').equal('10')
    env.expect('rdict.sdget', 'myDict', '0.5').equal('x')
    env.expect('rdict.sdget', 'myDict', '2').equal(None)

def test_sddel():
    env = Env()
    env.cmd('rdict.sdset', 'myDict', '0.5', '1')
    env.cmd('rdict.sdset', 'myDict', '1', '2')
    env.cmd('rdict.sdset', 'myDict', '2', '3')
    env.expect('rdict.sddel', 'myDict', '0.5', '0.7').equal(1)
    env.expect('rdict.sddel', 'myDict', '0.6').equal(0)
    env.expect('hlen', '{myDict}_sd1').equal(2)
    env.expect('zrange', '{myDict}_sd2', '0', '-1').equal(['1', '2'])
    env.expect('rdict.sddel', 'myDict', '1', '2').equal(2)
    env.expect('keys', '*').equal([])
    env.expect('rdict.sddel', 'myDict', '3').equal(0)

def test_sdadel():
    env = Env()
    env.cmd('rdict.sdset', 'myDict', '0.5', '1')
    env.cmd('rdict.sdset', 'myDict_2', '1', '2')
    env.cmd('rdict.sdset', 'myDict_3', '3', '4')
    env.assertEqual(sorted(env.expect('keys', '*').res), sorted(['{myDict}_sd1', '{myDict}_sd2', '{myDict_2}_sd1', '{myDict_2}_sd2', '{myDict_3}_sd1', '{myDict_3}_sd2']), 1)
    env.expect('rdict.sdadel', 'myDict_4', 'myDict_2').equal(1)
    env.assertEqual(sorted(env.expect('keys', '*').res), sorted(['{myDict}_sd1', '{myDict}_sd2', '{myDict_3}_sd1', '{myDict_3}_sd2']), 1)
    env.expect('rdict.sdadel', 'myDict', 'myDict_3').equal(2)
    env.expect('keys', '*').equal([])

def test_sdrange():
    env = Env()
    env.cmd('rdict.sdset', 'myDict', '1', '10')
    env.cmd('rdict.sdset', 'myDict', '0.5', '20')
    env.cmd('rdict.sdset', 'myDict', '2', '15')
    env.expect('rdict.sdrange', 'myDict', '0', '1').equal(['0.5', '20', '1', '10'])
    env.expect('rdict.sdrange', 'myDict', '0', '-1').equal(['0.5', '20', '1', '10', '2', '15'])
    env.expect('rdict.sdrange', 'myDict', '-1', '0').equal([])
    env.expect('rdict.sdrrange', 'myDict', '0', '0').equal(['2', '15'])
    env.expect('rdict.sdrrange', 'myDict', '0', '1').equal(['2', '15', '1', '10'])
    env.expect('rdict.sdrrange', 'myDict', '0', '-1').equal(['2', '15', '1', '10', '0.5', '20'])

def test_custom_hashslot():
    env = Env()
    env.cmd('rdict.sdset', '123_{myDict}', '1', '10')
    env.expect('keys', '*').contains('123_{myDict}_sd1')