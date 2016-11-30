import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.util.NavigableMap;
import java.util.TreeMap;

public class MergePerfMap {
    public static void main(String[] args) throws IOException {
        final NavigableMap<String, String> symbols = new TreeMap<>();

        final BufferedReader reader = new BufferedReader(new InputStreamReader(System.in));
        for (String line = reader.readLine(); line != null; line = reader.readLine()) {
            final int sep = line.indexOf(' ');
            if (sep == -1)
                throw new IllegalArgumentException("Wrong line format: " + line);
            symbols.put(line.substring(0, sep), line);
        }

        for (String value: symbols.values()) {
            System.out.println(value);
        }
    }
}
