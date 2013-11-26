package kiv.janecekz;

import java.io.IOException;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
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
	private final DatagramSocket ds;
	private final Presentation p;
	private final byte[] buffer;

	public Backend(DatagramSocket ds, Presentation p) {
		super();
		this.ds = ds;
		this.p = p;
		buffer = new byte[Presentation.MAX_PAKET_SIZE];
		m_names = new String[MAX_PLAYERS];
		m_colors = new int[MAX_PLAYERS];
		m_states = new int[MAX_PLAYERS];
	}

	public synchronized void listenStop() throws InterruptedException {
		listening = false;
	}
/*
	private void print_map() {
		int i, j;
		char c;

		for (i = 0; i < m_height; i++) {
			System.out.print(i+" ");
			for (j = 0; j < m_width; j++) {
				switch (mapData[i * m_width + j]) {
				case 11:
					c = '#';
					break;
				case 13:
					c = ' ';
					break;
				case 12:
					c = 'o';
					break;
				default:
					c = 's';
					break;
				}
				System.out.print(c);
			}
			System.out.printf("\n");
		}
		
		System.out.println("--------------------------\n");
	}
*/
	@Override
	public void run() {
		super.run();

		Presentation.PacketType t;

		// receive STATUS, DEAD, DISCONNECT

		DatagramPacket recv = new DatagramPacket(buffer, buffer.length);
		while (listening) {
			try {
				ds.receive(recv);
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
				p.sendPacket(PacketType.WAIT, null);
				javax.swing.SwingUtilities.invokeLater(p.wantStart);
				break;
			case DISCONNECT:
				javax.swing.SwingUtilities.invokeLater(p.disconnect);
				listening = false;
				break;
			default:
				break;
			}
		}

		//System.out.println("Backend thread exiting");
	}
}
