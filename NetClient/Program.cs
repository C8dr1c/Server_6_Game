using System;
using System.Linq;
using System.Net;
using System.Net.Sockets;

namespace NetClient
{
    class Program
    {
        static void Main(string[] args)
        {
            Console.WriteLine("**********************************************************");
            Console.WriteLine("*          Simple C# socket client application           *");
            Console.WriteLine("**********************************************************");
            Console.WriteLine("");
            Console.WriteLine("");

            // Lecture des paramètres en cours
            int port;
            if (args == null || args.Length != 2 || !int.TryParse(args[1], out port))
            {
                string file = typeof(Program).Assembly.Location;
                string appExeName = System.IO.Path.GetFileNameWithoutExtension(file);
                Console.WriteLine("Invalid parameters !");
                Console.WriteLine("program usage : " + appExeName + " server_address connection_port");
                return;
            }
            Console.WriteLine("Trying to open connection to server " + args[0] + " on the port " + args[1] + "...");

            var s = OpenConnection(args[0], port);
            if (s == null)
                return;
            Console.WriteLine("Connection to server opened successfully !");

            string buffer;
            while (true)
            {
                Console.WriteLine("");
                Console.Write(args[0] + "> ");
                buffer = Console.ReadLine();

                // Traitement de la chaîne lue
                if (string.IsNullOrEmpty(buffer))
                    continue;
                else if (buffer == "EXIT")
                    break;

                // Envoi du buffer au serveur
                Send(s, buffer);

                // Lecture de la réponse du serveur
                buffer = Receive(s);
                if (string.IsNullOrEmpty(buffer))
                    continue;

                // Traitement du résultat lu sur la socket
                if (buffer == "CONNECTION_CLOSED")
                {
                    Console.WriteLine("Server has closed connection !");
                    break;
                }
                else
                {
                    // Affichage du message
                    Console.WriteLine(buffer);
                }
            }

            CloseConnection(s);
        }

        private static string GetAddress(string serverAddress)
        {
            try
            {
                IPHostEntry iphostentry = Dns.GetHostEntry(serverAddress);
                return iphostentry.AddressList.FirstOrDefault(ip => ip.AddressFamily == AddressFamily.InterNetwork)?.ToString();
            }
            catch (SocketException e)
            {
                Console.WriteLine("Error while retreiving server ip from received server address : " + e.Message);
            }

            return "";
        }

        private static Socket OpenConnection(string serverAddress, int connectionPort)
        {
            Socket s;
            IPAddress ip = IPAddress.Parse(GetAddress(serverAddress));
            IPEndPoint ipEnd = new IPEndPoint(ip, connectionPort);
            s = new Socket(AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp);
            try
            {
                s.Connect(ipEnd);
                if (s.Connected)
                    return s;
            }
            catch (SocketException e)
            {
                Console.WriteLine("Error while openning connection to server : " + e.Message);
            }
            return null;
        }

        private static bool CloseConnection(Socket s)
        {
            if (s != null && s.Connected)
            {
                try
                {
                    s.Shutdown(SocketShutdown.Both);
                    s.Close();
                    if (s.Connected)
                        WriteError("Error while closing socket : ");
                    else
                        return true;
                }
                catch (SocketException e)
                {
                    Console.WriteLine("Error while closing socket : " + e.Message);
                }
            }
            return false;
        }

        private static string Receive(Socket s)
        {
            if (s == null || s.Connected || s.Poll(10, SelectMode.SelectRead) && s.Available == 0)
            {
                //La connexion a été clôturée par le serveur ou bien un problème
                //réseau est apparu
                Console.WriteLine("Connection to server has been closed.");
                return null;
            }

            // Attente de data à lire
            while (s.Available == 0) ;

            // Lecture des données
            string messageReceived = null;
            while (s.Available > 0)
            {
                try
                {
                    byte[] msg = new byte[s.Available];
                    //Réception des données
                    s.Receive(msg, 0, s.Available, SocketFlags.None);
                    var msgStr = System.Text.Encoding.UTF8.GetString(msg).Trim();
                    //On concatène les données reçues(max 4ko) dans
                    //une variable de la classe
                    messageReceived += msgStr;
                }
                catch (SocketException e)
                {
                    Console.WriteLine("Error while receiving data on socket : " + e.Message);
                }
            }
            return messageReceived;
        }

        private static void Send(Socket s, string message)
        {
            if (string.IsNullOrEmpty(message))
                return;

            if (s == null || !s.Connected || !s.Poll(10, SelectMode.SelectWrite))
            {
                //La connexion a été clôturée par le serveur ou bien un problème
                //réseau est apparu
                Console.WriteLine("Connection to server has been closed.");
                return;
            }

            byte[] msg = System.Text.Encoding.UTF8.GetBytes(message);

            try
            {
                s.Send(msg, msg.Length, SocketFlags.None);
            }
            catch (SocketException e)
            {
                Console.WriteLine("Error while sending data on socket : " + e.Message);
            }
        }

        private static void WriteError(string description)
        {
            Console.WriteLine(description + Convert.ToString(System.Runtime.InteropServices.Marshal.GetLastWin32Error()));
        }
    }
}
