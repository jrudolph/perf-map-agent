/*
 *   libperfmap: a JVM agent to create perf-<pid>.map files for consumption
 *               with linux perf-tools
 *   Copyright (C) 2013-2015 Johannes Rudolph<johannes.rudolph@gmail.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License along
 *   with this program; if not, write to the Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

package net.virtualvoid.perf;

import java.io.File;
import java.io.FileInputStream;

import java.text.MessageFormat;

import com.sun.tools.attach.VirtualMachine;
import java.lang.management.ManagementFactory;
import java.util.Locale;

public class AttachOnce {
    public static void main(String[] args) throws Exception {
        String pid = args[0];
        String options = "";
        if (args.length == 8) { // objdump
            assert(args[0].startsWith("--start-address="));
            String addr = args[0].substring("--start-address=".length());
            //long addrl = Long.parseLong(addr, 16); // sanity check
            options = "disasm="+addr;

            MessageFormat format = new MessageFormat("/tmp/perf-{0}.map");
            pid = (String) format.parse(args[7])[0];
            System.err.println("Got address "+addr+" and pid "+pid);
            loadAgent(pid, options);

            byte[] buffer = new byte[10000];
            FileInputStream fis = new FileInputStream("/tmp/test.output");
            int read = fis.read(buffer);
            while (read > 0) {
                System.out.write(buffer, 0, read);
                read = fis.read(buffer);
            }

            return;
        } else if (args.length > 1) {
            options = args[1];
        }
        loadAgent(pid, options);
    }

    static void loadAgent(String pid, String options) throws Exception {
        VirtualMachine vm = VirtualMachine.attach(pid);
        try {
            final File lib;
            if (System.getProperty("os.name", "").toLowerCase(Locale.US).contains("os x")) {
                lib = new File("libperfmap.dylib");
            } else {
                lib = new File("libperfmap.so");
            }
            String fullPath = lib.getAbsolutePath();
            if (!lib.exists()) {
                System.out.printf("Expected %s at '%s' but it didn't exist.\n", lib.getName(), fullPath);
                System.exit(1);
            }
            else vm.loadAgentPath(fullPath, options);
        } catch(com.sun.tools.attach.AgentInitializationException e) {
            // rethrow all but the expected exception
            if (!e.getMessage().equals("Agent_OnAttach failed")) throw e;
        } finally {
            vm.detach();
        }
    }
}
