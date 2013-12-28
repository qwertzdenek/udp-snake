/* Backend.java

UDP Snake
Copyright 2013 Zdeněk Janeček (ycdmdj@gmail.com)

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

package kiv.janecekz;

import java.io.IOException;
import java.net.DatagramPacket;
import java.util.StringTokenizer;

import kiv.janecekz.Presentation.PacketType;

/**
 * Datagram Socket backend thread. It waits for the packet from the server
 * and delegates changes to the Presentation thread.
 */
public class Backend extends Thread {
	public static final int MAX_PLAYERS = 16;

	/**
	 * loaded map data
	 */
	public byte[] mapData;
	public int m_width;  // map width
	public int m_height; // map height
	public int players;  // count of players
	public String[] m_names; // player names
	public int[] m_colors; // player colors
	public int[] m_states; // player score

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

	/**
	 * Breaks the thread loop.
	 */
	public synchronized void listenStop() throws InterruptedException {
		listening = false;
	}

	@Override
	public void run() {
		super.run();

		Presentation.PacketType t;

		// receive STATUS, DEAD, DISCONNECT, START

		DatagramPacket recv = new DatagramPacket(buffer, buffer.length);
		while (listening) {
			try {
				p.ds.receive(recv);
			} catch (IOException e) {
				break;
			}
			t = PacketType.values()[buffer[0]];

			switch (t) {
			case STATE:  // new game state
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
			case DEAD: // we are dead
				p.sendPacket(PacketType.WAIT, p.myServer, p.myPort, null);
				javax.swing.SwingUtilities.invokeLater(p.wantStart);
				break;
			case DISCONNECT: // We are disconnect
				javax.swing.SwingUtilities.invokeLater(p.disconnect);
				listening = false;
				break;
			case START: // Connection established
				javax.swing.SwingUtilities.invokeLater(p.conEst);
				p.myServer = recv.getAddress();
				p.myPort = recv.getPort();
				break;
			default:
				break;
			}
		}
	}
}
