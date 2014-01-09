/* Presentation.java

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

import java.awt.BorderLayout;
import java.awt.Color;
import java.awt.Dimension;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.event.KeyEvent;
import java.awt.event.KeyListener;
import java.awt.event.WindowEvent;
import java.awt.event.WindowListener;
import java.io.IOException;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.net.SocketException;
import java.net.UnknownHostException;
import java.util.Timer;
import java.util.TimerTask;

import javax.swing.BoxLayout;
import javax.swing.DefaultListModel;
import javax.swing.JButton;
import javax.swing.JComboBox;
import javax.swing.JComponent;
import javax.swing.JFrame;
import javax.swing.JLabel;
import javax.swing.JList;
import javax.swing.JOptionPane;
import javax.swing.JPanel;
import javax.swing.JTextField;
import javax.swing.UIManager;
import javax.swing.UnsupportedLookAndFeelException;

/**
 * Game window. Reads input data from the user and sends them to the server.
 */
public class Presentation extends JPanel implements ActionListener,
		KeyListener, WindowListener {
	private static final long serialVersionUID = 3965225930566059274L;
	private static final int MAX_NAME_LEN = 16;

	// list of top bar items
	private JComponent[] toolbar;
	private JLabel bottomLabel;
	private JButton loginButton;

	// game map
	private RenderPanel rp;
	private DefaultListModel<String> listModel;
	private Backend back;
	private boolean connected = false;
	private Timer conTimer;

	public static final int MAX_PAKET_SIZE = 1024;

	// connection socket
	public DatagramSocket ds = null;

	public InetAddress mainServer = InetAddress.getLoopbackAddress();
	public int mainPort = 10100;
	public InetAddress myServer = null;
	public int myPort = 0;

	

	private String[] colorStrings = { "Red", "Green", "Blue", "Brown",
			"Yellow", "Orange", "Purple", "Black", "Grey" };

	public Color bg = new Color(238, 229, 222);

	public Color[] awtColors = { Color.RED, Color.GREEN,
			Color.BLUE, new Color(139, 69, 19), Color.YELLOW, Color.ORANGE,
			new Color(160, 32, 240), Color.BLACK, new Color(190, 190, 190) };

	/**
	 * Packet types. It is first byte of packet.
	 */
	public enum PacketType {
		CONNECT, MOVE, START, STATE, DEAD, DISCONNECT, WAIT;

		private final byte b;

		private PacketType() {
			this.b = (byte) ordinal();
		}

		public byte getSymbol() {
			return b;
		}
	}

	/**
	 * Asynchronous task from the Backend to update map data
	 */
	public Runnable updateMap = new Runnable() {
		@Override
		public void run() {
			listModel.clear();
			for (int i = 0; i < back.players; i++) {
				listModel.addElement(back.m_names[i] + " - " + back.m_states[i]);
			}

			rp.updateMap();
			rp.repaint();
		}
	};

	/**
	 * Backend task to indicate connected server.
	 */
	public Runnable wantStart = new Runnable() {
		@Override
		public void run() {
			for (JComponent cm : toolbar) {
				cm.setEnabled(false);
			}
			bottomLabel.setText("Pro připojení do hry stiskni mezerník.");
		}
	};

	/**
	 * Disconnects us enables components.
	 */
	public Runnable disconnect = new Runnable() {
		@Override
		public void run() {
			connected = false;
			for (JComponent cm : toolbar) {
				cm.setEnabled(true);
			}
			bottomLabel.setText("Server ukončil spojení");
			loginButton.setText("Připojit");
			listModel.clear();
			rp.clear();
		}
	};

	/**
	 * Executed when was received confirmation packet from the server.
	 */
	public Runnable conEst = new Runnable() {
		
		@Override
		public void run() {
			connected = true;
			loginButton.setText("Odpojit");
			loginButton.setEnabled(true);
		}
	};
	
	/**
	 * Sends packet to the target system.
	 * @param t type of the packet
	 * @param mAddress destination address
	 * @param mPort destination port
	 * @param aditional additional informations like username
	 */
	public void sendPacket(PacketType t, InetAddress mAddress, int mPort,
			String aditional) {
		byte[] buffer = new byte[22];
		int len = 0;
		
		if (mAddress == null)
			return;

		try {
			buffer[len++] = (byte) t.getSymbol();

			switch (t) {
			case CONNECT:
				String[] in = aditional.split(",");
				byte[] name = in[0].getBytes("UTF-8");
				byte[] color = in[1].getBytes();
				System.arraycopy(name, 0, buffer, len,
						Math.min(MAX_NAME_LEN, name.length));
				len += Math.min(MAX_NAME_LEN, name.length) + 1;
				System.arraycopy(color, 0, buffer, len, color.length);
				len += color.length + 1;
				break;
			case START:
				// only head
				break;
			case MOVE:
				buffer[len++] = (byte) aditional.charAt(0);
				break;
			case DISCONNECT:
				// only head
				break;
			case WAIT:
				// only head
				break;
			default:
				break;
			}

			DatagramPacket send = new DatagramPacket(buffer, len, mAddress,
					mPort);
			ds.send(send);

		} catch (IOException e) {
			System.out.println("Socket IO error");
		}
	}

	public Presentation() {
		super();
		toolbar = new JComponent[3];

		setFocusable(true);
		requestFocusInWindow();

		// tool panel
		JPanel toolPanel = new JPanel();
		toolPanel.setLayout(new BoxLayout(toolPanel, BoxLayout.X_AXIS));
		JTextField serverAdress = new JTextField();
		serverAdress.setText("localhost:9700");
		toolbar[0] = serverAdress;
		JTextField userName = new JTextField();
		userName.setText("jmeno");
		toolbar[1] = userName;
		JComboBox<String> colors = new JComboBox<String>(colorStrings);
		colors.setSelectedIndex(0);
		colors.addActionListener(this);
		toolbar[2] = colors;

		loginButton = new JButton();
		loginButton.setText("Připojit");
		loginButton.addActionListener(this);

		toolPanel.add(serverAdress);
		toolPanel.add(userName);
		toolPanel.add(colors);
		toolPanel.add(loginButton);

		// playground
		rp = new RenderPanel(this);

		// player list
		listModel = new DefaultListModel<String>();
		JList<String> players = new JList<String>(listModel);
		players.setPreferredSize(new Dimension(200, 300));
		players.setEnabled(false);

		bottomLabel = new JLabel("Zdeněk Janeček 2013");

		setLayout(new BorderLayout());
		add(toolPanel, BorderLayout.PAGE_START);
		add(rp, BorderLayout.LINE_START);
		add(players, BorderLayout.LINE_END);
		add(bottomLabel, BorderLayout.PAGE_END);

		addKeyListener(this);
		
		conTimer = new Timer();
	}

	@SuppressWarnings("unchecked")
    @Override
	public void actionPerformed(ActionEvent e) {
		if (e.getSource().equals(loginButton)) {
	        requestFocus();
			if (connected) {
				sendPacket(PacketType.DISCONNECT, myServer, myPort, null);
				connected = false;
				for (JComponent cm : toolbar) {
					cm.setEnabled(true);
				}
				loginButton.setText("Připojit");
				bottomLabel.setText("Zadejte nové spojení");
				listModel.clear();
				conTimer.purge();
			} else {
				String address = ((JTextField) toolbar[0]).getText();
				String name = ((JTextField) toolbar[1]).getText();
				if (name.length() == 0) {
					JOptionPane.showMessageDialog(this, "Prázdné jméno");
					return;
				}

				int color = 50 + ((JComboBox<String>) toolbar[2])
						.getSelectedIndex();

				String s_name = address.split(":")[0];
				try {
					mainPort = Integer.parseInt(address.split(":")[1]);
					mainServer = InetAddress.getByName(s_name);
				} catch (NumberFormatException e2) {
					JOptionPane.showMessageDialog(this, "Neplatný port");
					return;
				} catch (IndexOutOfBoundsException e3) {
					JOptionPane.showMessageDialog(this,
							"Pište adresu ve formátu IP:port (pouze IPv4)");
					return;
				} catch (UnknownHostException e1) {
					JOptionPane.showMessageDialog(this, "Server nenalezen");
					return;
				}

				try {
					cleanUp();

					ds = new DatagramSocket();

					back = new Backend(this);
					back.start();

				} catch (SocketException e1) {
					System.out.println("DatagramSocket error");
				}

				sendPacket(PacketType.CONNECT, mainServer, mainPort, name + ","
						+ color);
				
				loginButton.setEnabled(false);
				
				conTimer.schedule(new TimerTask() {
					@Override
					public void run() {
						if (!connected) {
							JOptionPane.showMessageDialog(getParent(),
									"Server neodpovídá");
							loginButton.setEnabled(true);
						}
					}
				}, 400);
			}
		}
	}

	@Override
	public void keyPressed(KeyEvent arg0) {
		if (!connected)
			return;

		int kc = arg0.getKeyCode();

		if (kc == KeyEvent.VK_SPACE) {
			sendPacket(PacketType.START, myServer, myPort, null);
			bottomLabel.setText("Hra");
		} else if (kc == KeyEvent.VK_KP_LEFT || kc == KeyEvent.VK_LEFT) {
			sendPacket(PacketType.MOVE, myServer, myPort, "L");
		} else if (kc == KeyEvent.VK_KP_RIGHT || kc == KeyEvent.VK_RIGHT) {
			sendPacket(PacketType.MOVE, myServer, myPort, "R");
		} else if (kc == KeyEvent.VK_KP_UP || kc == KeyEvent.VK_UP) {
			sendPacket(PacketType.MOVE, myServer, myPort, "T");
		} else if (kc == KeyEvent.VK_KP_DOWN || kc == KeyEvent.VK_DOWN) {
			sendPacket(PacketType.MOVE, myServer, myPort, "D");
		}
	}

	@Override
	public void keyReleased(KeyEvent arg0) {
		// not used
	}

	@Override
	public void keyTyped(KeyEvent arg0) {
		// not used
	}

	public Backend getBackend() {
		return back;
	}

	/**
	 * Cleans up actual connection
	 */
	public void cleanUp() {
		try {
			if (back != null && back.isAlive()) {
				back.listenStop();
				sendPacket(PacketType.DISCONNECT, myServer, myPort, null);
				ds.close();
				back.join();
				ds = null;
			} else if (ds != null) {
			    ds.close();
			}
		} catch (InterruptedException e) {
		    // it should not happen
			e.printStackTrace();
		}
	}

	/**
	 * Create the GUI and show it. For thread safety, this method should be
	 * invoked from the event-dispatching thread.
	 */
	private static void createAndShowGUI() {
		// Create and set up the window.
		JFrame frame = new JFrame("UDP Snake Client");
		frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);

		// Create and set up the content pane.
		JComponent newContentPane = new Presentation();
		newContentPane.setOpaque(true); // content panes must be opaque
		frame.setContentPane(newContentPane);

		frame.addWindowListener((Presentation) newContentPane);

		// Display the window.
		frame.pack();
		frame.setVisible(true);
	}

	public static void main(String[] args) {
		try {
			UIManager.setLookAndFeel(UIManager
					.getCrossPlatformLookAndFeelClassName());
		} catch (ClassNotFoundException | InstantiationException
				| IllegalAccessException | UnsupportedLookAndFeelException e) {
			e.printStackTrace();
		}

		;
		// Schedule a job for the event-dispatching thread:
		// creating and showing this application's GUI.
		javax.swing.SwingUtilities.invokeLater(new Runnable() {
			public void run() {
				createAndShowGUI();
			}
		});
	}

	@Override
	public void windowActivated(WindowEvent arg0) {
	}

	@Override
	public void windowClosed(WindowEvent arg0) {
	}

	@Override
	public void windowClosing(WindowEvent arg0) {
		cleanUp();
	}

	@Override
	public void windowDeactivated(WindowEvent arg0) {
	}

	@Override
	public void windowDeiconified(WindowEvent arg0) {
	}

	@Override
	public void windowIconified(WindowEvent arg0) {
	}

	@Override
	public void windowOpened(WindowEvent arg0) {
	}
}
