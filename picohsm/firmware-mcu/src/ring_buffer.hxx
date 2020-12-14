/**
 * This file is part of picoHSM
 * 
 * picoHSM is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * Copyright 2020 Ledger SAS, written by Olivier Hériveaux
 */


/**
 * A circular buffer to receiving bytes from UART or other peripherals.
 */
template <uint32_t Size> class ring_buffer_t
{
    public:
        /**
         * Default constructor.
         */
        ring_buffer_t():
            write(0),
            read(0)
        {}

        /**
         * Write a byte in the buffer.
         * @param data Byte to be written.
         * @return false if the buffer is full, true otherwise.
         */
        bool put(uint8_t data)
        {
            uint8_t write_next = next(write);
            if (write_next == read)
            {
                /* Buffer is full. Drop byte. */
                return false;
            }
            else
            {
                buffer[write] = data;
                write = write_next;
                return true;
            }
        }

        /**
         * Pop a byte from the buffer. Blocks until a byte is
         * available.
         * @return Data byte.
         */
        uint8_t pop()
        {
            while (read == write){}
            uint8_t value = buffer[read];
            read = next(read);
            return value;
        }

        /**
         * @return true if data is available.
         */
        bool has_data() const
        {
            return (read != write);
        }

        /**
         * Removes all items from the buffer.
         */
        void flush()
        {
            read = write = 0;
        }

        /**
         * @return Number of available bytes.
         */
        uint32_t size() const
        {
            if (write >= read)
                return write - read;
            else
                return (Size - read) + write;
        }

        /**
         * @return Byte at a given offset in the buffer. Used to read data
         *     without poping it. Returns 0 if the offset is too big.
         * @param offset Offset in the buffer.
         */
        uint8_t at(uint32_t offset) const
        {
            if (offset >= size())
                return 0;
            uint32_t index = ((uint32_t)read + (uint32_t)offset) % Size;
            return buffer[index];
        }

    private:
        /** Where the received bytes are stored. */
        uint8_t buffer[Size];
        /** Index of the next byte to be written. */
        volatile uint32_t write;
        /** Index of the next byte to be read.
         * If equal to write, this means no bytes are available. */
        volatile uint32_t read;

        /**
         * Next value of an index in the circular buffer.
         * @param current Current index.
         * @return Next index.
         */
        uint32_t next(uint32_t current) const
        {
            return (current + 1) % Size;
        }
};

