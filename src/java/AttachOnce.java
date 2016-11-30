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

import com.sun.tools.attach.VirtualMachine;
import com.sun.tools.javac.code.Attribute;

import java.lang.management.ManagementFactory;
import java.util.Arrays;

public class AttachOnce {
    public static void main(String[] args) throws Exception {
        if (args.length < 2) {
            System.err.println(
                    "Usage:" +
                    "\tjava ... <PID> <*.so> [<options>]");
            System.exit(-1);
        }

        final String pid = args[0];
        final String so = args[1];
        final CharSequence[] options = Arrays.copyOfRange(args, 2, args.length);

        loadAgent(pid, so, String.join(" ", options));
    }

    static void loadAgent(final String pid, final String so, final String options) throws Exception {
        final VirtualMachine vm = VirtualMachine.attach(pid);
        try {
            final File lib = new File(so);
            final String fullPath = lib.getAbsolutePath();
            if (!lib.exists()) {
                System.out.printf("Expected '%s' at '%s' but it didn't exist.\n", so, fullPath);
                System.exit(1);
            }

            vm.loadAgentPath(fullPath, options);
        } finally {
            vm.detach();
        }
    }
}
