package kiv.janecekz;

import java.io.IOException;
import java.net.DatagramPacket;
import java.util.StringTokenizer;

import kiv.janecekz.Presentation.PacketType;

public class Backend extends Thread {
	public static final int MAX_PLAYERS = 10;

	public byte[] mapData;
	public int m_width;
	public int m_height;
	public int players;
	public String[] m_names;
	public int[] m_colors;
	public int[] m_states;

	private boolean listening = true;
	private final Presentation p;
	private final byte[] buffer;

	public Backend(Presentation p) {
		super();
		this.p = p;
		buffer = new byte[Presentation.MAX_PAKET_SIZE];
		m_names = new String[MAX_PLAYERS];
		m_colors = new int[MAX_PLAYERS];
		m_states = new int[MAX_PLAYERS];
	}

	public synchronized void listenStop() throws InterruptedException {
		listening = false;
	}

	@Override
	public void run() {
		super.run();

		Presentation.PacketType t;

		// receive STATUS, DEAD, DISCONNECT

		DatagramPacket recv = new DatagramPacket(buffer, buffer.length);
		while (listening) {
			try {
				p.ds.receive(recv);
			} catch (IOException e) {
				//System.out.println("Neplatný socket");
				break;
			}
			t = PacketType.values()[buffer[0]];

			switch (t) {
			case STATE:
				String inData = new String(buffer, 2, recv.getLength() - 2);
				StringTokenizer token = new StringTokenizer(inData, "\0");
				players = buffer[1];
				int id;
				for (int i = 0; i < players; i++) {
					id = Integer.parseInt(token.nextToken());
					m_names[id] = token.nextToken();
					m_colors[id] = Integer.parseInt(token.nextToken());
					m_states[id] = Integer.parseInt(token.nextToken());
				}

				m_width = Integer.parseInt(token.nextToken());
				m_height = Integer.parseInt(token.nextToken());

				mapData = token.nextToken().getBytes();

				javax.swing.SwingUtilities.invokeLater(p.updateMap);
				break;
			case DEAD:
				p.sendPacket(PacketType.WAIT, p.myServer, p.myPort, null);
				javax.swing.SwingUtilities.invokeLater(p.wantStart);
				break;
			case DISCONNECT:
				javax.swing.SwingUtilities.invokeLater(p.disconnect);
				listening = false;
				break;
			case START:
				javax.swing.SwingUtilities.invokeLater(p.conEst);
				p.myServer = recv.getAddress();
				p.myPort = recv.getPort();
				break;
			default:
				break;
			}
		}

		//System.out.println("Backend thread exiting");
	}
}
