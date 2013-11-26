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

public class Presentation extends JPanel implements ActionListener,
		KeyListener, WindowListener {
	private static final long serialVersionUID = 3965225930566059274L;
	private static DatagramSocket ds = null;

	public static final int MAX_PAKET_SIZE = 1024;

	private JComponent[] toolbar;
	private JLabel bottomLabel;
	private RenderPanel rp;
	private DefaultListModel<String> listModel;
	private InetAddress server = InetAddress.getLoopbackAddress();
	private Backend back;
	private int port = 10100;
	
	protected boolean connected = false;

	private String[] colorStrings = { "White", "Red", "Green", "Blue", "Brown",
			"Yellow", "Orange", "Purple", "Black", "Grey" };

	public Color bg = new Color(238, 229, 222);

	public Color[] awtColors = { Color.WHITE, Color.RED, Color.GREEN,
			Color.BLUE, new Color(139, 69, 19), Color.YELLOW, Color.ORANGE,
			new Color(160, 32, 240), new Color(190, 190, 190) };

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

	public Runnable wantStart = new Runnable() {
		@Override
		public void run() {
			connected = true;
			for (JComponent cm : toolbar) {
				cm.setEnabled(false);
			}
			bottomLabel.setText("Pro připojení do hry stiskni mezerník.");
		}
	};

	public Runnable disconnect = new Runnable() {
		@Override
		public void run() {
			connected = false;
			for (JComponent cm : toolbar) {
				cm.setEnabled(true);
			}
			bottomLabel.setText("Server ukončil spojení");
		}
	};

	public void sendPacket(PacketType t, String aditional) {
		byte[] buffer = new byte[16];
		int len = 0;

		try {
			buffer[len++] = (byte) t.getSymbol();

			switch (t) {
			case CONNECT:
				String[] in = aditional.split(",");
				String name = in[0];
				String color = in[1];
				System.arraycopy(name.getBytes(), 0, buffer, Math.min(len, 10),
						name.length());
				len += name.length() + 1;
				System.arraycopy(color.getBytes(), 0, buffer, len,
						color.length());
				len += color.length() + 1;
				break;
			case START:

				break;
			case MOVE:
				buffer[len++] = (byte) aditional.charAt(0);
				break;
			case DISCONNECT:

				break;
			case WAIT:
				
				break;
			default:

				break;
			}

			DatagramPacket send = new DatagramPacket(buffer, len, server, port);
			ds.send(send);

		} catch (IOException e) {
			System.out.println("Socket IO error");
		}
	}

	public Presentation() {
		super();
		toolbar = new JComponent[4];

		setFocusable(true);
		requestFocusInWindow();

		// tool panel
		JPanel toolPanel = new JPanel();
		toolPanel.setLayout(new BoxLayout(toolPanel, BoxLayout.X_AXIS));
		JTextField serverAdress = new JTextField();
		serverAdress.setText("10.77.96.74:10100");
		toolbar[0] = serverAdress;
		JTextField userName = new JTextField();
		userName.setText("jmeno");
		toolbar[1] = userName;
		JComboBox<String> colors = new JComboBox<String>(colorStrings);
		colors.setSelectedIndex(0);
		colors.addActionListener(this);
		toolbar[2] = colors;
		JButton login = new JButton();
		login.setText("Přihlásit");
		toolbar[3] = login;
		login.addActionListener(this);

		toolPanel.add(serverAdress);
		toolPanel.add(userName);
		toolPanel.add(colors);
		toolPanel.add(login);

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
	}

	@Override
	public void actionPerformed(ActionEvent e) {
		if (e.getSource().equals(toolbar[3])) {
			String address = ((JTextField) toolbar[0]).getText();
			String name = ((JTextField) toolbar[1]).getText();
			int color = 50 + ((JComboBox<String>) toolbar[2])
					.getSelectedIndex();

			String s_name = address.split(":")[0];
			try {
				this.port = Integer.parseInt(address.split(":")[1]);
				this.server = InetAddress.getByName(s_name);
			} catch (NumberFormatException e2) {
				JOptionPane.showMessageDialog(this, "Neplatný port");
				return;
			} catch (IndexOutOfBoundsException e3) {
				JOptionPane.showMessageDialog(this,
						"Pište adresu ve formátu IP:port");
				return;
			} catch (UnknownHostException e1) {
				JOptionPane.showMessageDialog(this, "Server nenalezen");
				return;
			}

			try {
				cleanUp();

				ds = new DatagramSocket();

				back = new Backend(ds, this);
				back.start();

				Thread.sleep(200);
			} catch (InterruptedException e1) {
				e1.printStackTrace();
			} catch (SocketException e1) {
				System.out.println("DatagramSocket error");
			}
			
			sendPacket(PacketType.CONNECT, name + "," + color);
			
			new Timer().schedule(new TimerTask() {
				@Override
				public void run() {
					if (!connected)
						JOptionPane.showMessageDialog(getParent(), "Server neodpovídá");
				}
			}, 2000);
		}
	}

	@Override
	public void keyPressed(KeyEvent arg0) {
		int kc = arg0.getKeyCode();

		if (kc == KeyEvent.VK_SPACE) {
			sendPacket(PacketType.START, null);
			bottomLabel.setText("Hra");
		} else if (kc == KeyEvent.VK_KP_LEFT || kc == KeyEvent.VK_LEFT) {
			sendPacket(PacketType.MOVE, "L");
		} else if (kc == KeyEvent.VK_KP_RIGHT || kc == KeyEvent.VK_RIGHT) {
			sendPacket(PacketType.MOVE, "R");
		} else if (kc == KeyEvent.VK_KP_UP || kc == KeyEvent.VK_UP) {
			sendPacket(PacketType.MOVE, "T");
		} else if (kc == KeyEvent.VK_KP_DOWN || kc == KeyEvent.VK_DOWN) {
			sendPacket(PacketType.MOVE, "D");
		}
	}

	@Override
	public void keyReleased(KeyEvent arg0) {
		// TODO Auto-generated method stub

	}

	@Override
	public void keyTyped(KeyEvent arg0) {
		// TODO Auto-generated method stub

	}

	public Backend getBackend() {
		return back;
	}

	public void cleanUp() {
		try {
			if (back != null && back.isAlive()) {
				back.listenStop();
				sendPacket(PacketType.DISCONNECT, null);
				ds.close();
				back.join();
			} else if (ds != null) {
				ds.close();
			}
		} catch (InterruptedException e) {
			// TODO Auto-generated catch block
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

		System.out.println("Client is now starting");
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
