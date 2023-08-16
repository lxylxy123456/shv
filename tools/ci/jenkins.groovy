/*
 * SHV - Small HyperVisor for testing nested virtualization in hypervisors
 * Copyright (C) 2023  Eric Li
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

void build_shv(String subarch, String build_opts) {
    // PWD = sh(returnStdout: true, script: 'pwd').trim()
    sh "git clean -Xdf"
    // TODO: sh "./tools/ci/build.sh ${subarch} ${build_opts}"
    sh "autoreconf --install && \
        ./configure --host=${subarch}-linux-gnu ${build_opts} && \
        make -j \$(nproc)"
}

return this
