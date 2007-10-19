/* 
* Copyright (C) 2007, Brian Tanner
* 
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
* 
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
* 
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA. */

package rlglue.agent;
import rlglue.network.Network;

public class AgentLoader
{
	public static void main(String [] args) throws Exception
	{
		String usage = "java AgentLoader <Agent> -classpath <Path To RLGlue>";

		String envVars = "The following environment variables are used by the agent to control its function:\n" + 
		"RLGLUE_HOST  : If set the agent will use this ip or hostname to connect to rather than " + Network.kDefaultHost + "\n" + 
		"RLGLUE_PORT  : If set the agent will use this port to connect on rather than " + Network.kDefaultPort + "\n" +
		"RLGLUE_AUTORECONNECT  : If set the agent will reconnect to the glue after an experiment has finished\n";

		if (args.length < 1) {
			System.out.println(usage);
			System.out.println(envVars);
			System.exit(1);
		}

		Agent agent = (Agent)Class.forName(args[0]).newInstance();
		ClientAgent client = new ClientAgent(agent);
		int autoReconnect = 0;

		String host = Network.kDefaultHost;
		int port = Network.kDefaultPort;

		String hostString = System.getenv("RLGLUE_HOST");
		String portString = System.getenv("RLGLUE_PORT");
		String reconnect = System.getenv("RLGLUE_AUTORECONNECT");

		if (hostString != null)
			host = hostString;

		try {
			port = Integer.parseInt(portString);
		}
		catch (Exception e) {
			port = Network.kDefaultPort;
		}

		try {
			autoReconnect = Integer.parseInt(reconnect);
		}
		catch (Exception e) {
			autoReconnect = 0;
		}

		System.out.println("Connecting to " + host + " on port " + port + " with autoreconnect=" + autoReconnect);

		do {
			client.connect(host, port, Network.kRetryTimeout);
			client.runAgentEventLoop();
			client.close();
		} while (autoReconnect == 1);
	}
}
