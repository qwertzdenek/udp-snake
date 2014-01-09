/* RenderPanel.java

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

import java.awt.Color;
import java.awt.Dimension;
import java.awt.Graphics;

import javax.swing.JPanel;

/**
 * Custom component for the game map.
 */
public class RenderPanel extends JPanel {
	private static final long serialVersionUID = 6781744591023615562L;
	public static final int CANVAS_WIDTH = 600;
	public static final int CANVAS_HEIGHT = 600;

	private Backend back;
	private final Presentation p;
	
	private int m_width;
	private int m_height;
	private byte[] mapData;
	private int[] m_colors;

	public RenderPanel(Presentation p) {
		this.p = p;
		setBackground(p.bg);
		setPreferredSize(new Dimension(CANVAS_WIDTH, CANVAS_HEIGHT));
	}

	@Override
	protected void paintComponent(Graphics g) {
		super.paintComponent(g);
		
		if (mapData == null) {
		    g.setColor(p.bg);
            g.fillRect(0, 0, CANVAS_WIDTH, CANVAS_HEIGHT);
			return;
		}
		
		int grid_w = CANVAS_WIDTH / m_width;
		int grid_h = CANVAS_HEIGHT / m_width;
		
		int squareVal;
		Color c = p.bg;
		
		int startx = 0;
		int starty = 0;
		
		for (int i = 0; i < m_height; i++) {
			for (int j = 0; j < m_width; j++) {
				squareVal = mapData[i * m_width + j];
				if (squareVal <= Backend.MAX_PLAYERS) {
					c = p.awtColors[m_colors[squareVal - 1] - 50];
				} else if (squareVal == Backend.MAX_PLAYERS + 1) {
					c = Color.BLACK;
				} else if (squareVal == Backend.MAX_PLAYERS + 2) {
					c = Color.MAGENTA;
				} else {
					c = p.bg;
				}

				g.setColor(c);
				g.fillRect(startx, starty, grid_w, grid_h);
				
				startx += grid_w;
			}
			
			startx = 0;
			starty += grid_h;
		}
	}

   /**
    * Clears actual map.
    */
	public void clear() {
		mapData = null;
		repaint();
	}
	
   /**
    * Fetch accepted data.
    */
	public void updateMap() {
		back = p.getBackend();
		
		m_width = back.m_width;
		m_height = back.m_height;
		mapData = back.mapData;
		m_colors = back.m_colors;
	}
}
