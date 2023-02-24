#!/usr/bin/env python
from mininet.cli import CLI
from mininet.net import Mininet
from mininet.link import TCLink
from mininet.topo import Topo
from mininet.log import setLogLevel


class Networks(Topo):
    def __init__(self, **opts):
        Topo.__init__(self, **opts)
        h1 = self.addHost('h1')
        h2 = self.addHost('h2')
        h3 = self.addHost('h3')
        h4 = self.addHost('h4')
        h5 = self.addHost('h5')
        h6 = self.addHost('h6')
        h7 = self.addHost('h7')
        s1 = self.addSwitch('s1')
        self.addLink(h1, s1)
        self.addLink(h2, s1)
        self.addLink(h3, s1)
        self.addLink(h4, s1)
        self.addLink(h5, s1)
        self.addLink(h6, s1)
        self.addLink(h7, s1)

if __name__ == '__main__':
    setLogLevel('info')

    # Create data network
    topo = Networks()
    net = Mininet(topo=topo, link=TCLink, autoSetMacs=True, autoStaticArp=True)

    # Run network
    net.start()
    CLI(net)
    net.stop()
