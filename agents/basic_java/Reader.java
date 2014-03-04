/**
 * Needed to do reading from stdin in Java
 * SERIOUSLY
 * Stupid Java
 * @author Some website somewhere
 */
import java.io.*;
import java.lang.Exception;
import java.util.Vector;

class Reader
{
	public static String readLine()
	{
		String s = "";
		try 
		{
			InputStreamReader converter = new InputStreamReader(System.in);
			BufferedReader in = new BufferedReader(converter);
			s = in.readLine();
		} 
		catch (Exception e) 
		{
			System.out.println("EXCEPTION: Reader.readLine - "+e); 
		}
		return s;
	}

	public static Vector<String> readTokens(String r)
	{
		// String r = readLine();
		String token = "";
		Vector<String> result = new Vector<String>();
		for (int ii=0; ii < r.length(); ++ii)
		{
			if (r.charAt(ii) == ' ' || r.charAt(ii) == '\n')
			{
				result.add(new String(token));	
				//System.out.println("Token " + token);
				token = "";
			}
			else
				token += r.charAt(ii);
		}
		result.add(new String(token));
		return result;
	}
}
