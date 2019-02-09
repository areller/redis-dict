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

def test_sdset_multiple():
    env = Env()
    env.cmd('rdict.sdset', 'myDict', '0.15', '10')
    env.cmd('rdict.sdset', 'myDict', '0.05', '13')
    env.cmd('rdict.sdset', 'myDict', '0.2', '20')
    env.expect('zrange', '{myDict}_sd2', '0', '-1').equal(['0.05', '0.15', '0.2'])
    env.expect('hlen', '{myDict}_sd1').equal(3)
    env.expect('hget', '{myDict}_sd1', '0.15').equal('10')
    env.expect('hget', '{myDict}_sd1', '0.05').equal('13')
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
    env.expect('rdict.sddel', 'myDict', '0.5').equal(1)
    env.expect('rdict.sddel', 'myDict', '0.6').equal(0)
    env.expect('hgetall', '{myDict}_sd1').equal(['1', '2'])
    env.expect('zrange', '{myDict}_sd2', '0', '-1').equal(['1'])
    env.expect('rdict.sddel', 'myDict', '1').equal(1)
    env.expect('keys', '*').equal([])
    env.expect('rdict.sddel', 'myDict', '3').equal(0)

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