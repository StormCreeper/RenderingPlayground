import numpy as np
import matplotlib.pyplot as plt
from scipy.optimize import curve_fit
import sys

def load_csv(file_path): # load sensitivity functions in XYZ CIE space 
    try:
        data = np.loadtxt(file_path, delimiter=',', skiprows=0)
        return data[:, 0] * 1e-9, data[:, 1], data[:, 2], data[:, 3] # wavelength is in nm
    except Exception as e:
        print(f"Error loading file: {e}")
        return None, None, None, None

def gaussian(x, mu, sigma, A):
    return A * np.exp(-0.5 * ((x - mu) / sigma) ** 2)

def gaussian2(x, mu1, sigma1, A1, mu2, sigma2, A2):
    return gaussian(x, mu1, sigma1, A1) + gaussian(x, mu2, sigma2, A2)

def gaussianFit(x, y, two=False):  
    mu = np.sum(x * y) / np.sum(y)
    sigma = np.sqrt(np.abs(np.sum(y * (x - mu) ** 2) / np.sum(y)))
    A = np.max(y)
    print(f"mu: {mu}, sigma: {sigma}, A: {A}")
    if two:
        popt, _ = curve_fit(gaussian2, x, y, p0=[mu, sigma, A, mu*1.5, sigma, A])
    else:
        mu = np.sum(x * y) / np.sum(y)
        sigma = np.sqrt(np.abs(np.sum(y * (x - mu) ** 2) / np.sum(y)))
        A = np.max(y)
        popt, _ = curve_fit(gaussian, x, y, p0=[mu, sigma, A])
    return popt

def plot_xyz(wavelengths, X, Y, Z):

    X_fit = gaussianFit(wavelengths, X, two=True)
    Y_fit = gaussianFit(wavelengths, Y)
    Z_fit = gaussianFit(wavelengths, Z)

    plt.figure(figsize=(10, 5))
    plt.plot(wavelengths, X, label='X', color='red')
    plt.plot(wavelengths, Y, label='Y', color='green')
    plt.plot(wavelengths, Z, label='Z', color='blue')
    plt.plot(wavelengths, gaussian2(wavelengths, *X_fit), label='X Fit', color='red', linestyle='--')
    plt.plot(wavelengths, gaussian(wavelengths, *Y_fit), label='Y Fit', color='green', linestyle='--')
    plt.plot(wavelengths, gaussian(wavelengths, *Z_fit), label='Z Fit', color='blue', linestyle='--')
    plt.xlabel('Wavelength (m)')
    plt.ylabel('Values')
    plt.title('XYZ Values vs Wavelength')
    plt.legend()
    plt.grid(True)

# Sj = sj / (||sj|| * nu²)
def Sj(sj, nu):
    mean = np.max(sj)
    print(f"Mean: {mean}")
    sj = sj / (mean * nu ** 2)
    return sj

def plot_xyz_nu(lambda_, X, Y, Z):
    nu = 1 / lambda_  # Convert wavelength to frequency domain 
    # Normalize the sensitivity functions
    X_nu = Sj(X, nu)
    Y_nu = Sj(Y, nu)
    Z_nu = Sj(Z, nu)

    X_nu_fit = gaussianFit(nu, X_nu, two=True)
    Y_nu_fit = gaussianFit(nu, Y_nu)
    Z_nu_fit = gaussianFit(nu, Z_nu)
    
    plt.figure(figsize=(10, 5))
    plt.plot(nu, X_nu, label='X(nu)', color='red')
    plt.plot(nu, Y_nu, label='Y(nu)', color='green')
    plt.plot(nu, Z_nu, label='Z(nu)', color='blue')
    plt.plot(nu, gaussian2(nu, *X_nu_fit), label='X(nu) Fit', color='red', linestyle='--')
    plt.plot(nu, gaussian(nu, *Y_nu_fit), label='Y(nu) Fit', color='green', linestyle='--')
    plt.plot(nu, gaussian(nu, *Z_nu_fit), label='Z(nu) Fit', color='blue', linestyle='--')
    plt.xlabel('Frequency (1/m)')
    plt.ylabel('Scaled Sensitivity')
    plt.title('Scaled XYZ Sensitivity vs Frequency')
    plt.legend()
    plt.grid(True)

def plot_xyz_nu_fft(lambda_, X, Y, Z):
    nu = 1 / lambda_  # Convert wavelength to frequency domain
    # Normalize the sensitivity functions
    X_nu = Sj(X, nu)
    Y_nu = Sj(Y, nu)
    Z_nu = Sj(Z, nu)

    # Compute the FFT of the sensitivity functions
    X_fft = np.fft.fft(X_nu)
    Y_fft = np.fft.fft(Y_nu)
    Z_fft = np.fft.fft(Z_nu)

    # Compute the frequency domain
    freq = np.fft.fftfreq(len(nu), nu[1] - nu[0])
    
    plt.figure(figsize=(10, 5))
    plt.plot(freq, np.abs(X_fft), label='X(FFT)', color='red')
    plt.plot(freq, np.abs(Y_fft), label='Y(FFT)', color='green')
    plt.plot(freq, np.abs(Z_fft), label='Z(FFT)', color='blue')
    plt.xlabel('Frequency (1/m)')
    plt.ylabel('Magnitude')
    plt.title('FFT of XYZ Sensitivity vs Frequency')
    plt.legend()
    plt.grid(True)

def gauss(x, mu, sigma, A):
    return A * np.exp(-0.5 * ((x - mu) / sigma) ** 2)

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Usage: python script.py <csv_file>")
        sys.exit(1)
    
    file_path = sys.argv[1]
    lambda_, X, Y, Z = load_csv(file_path)

    # curve fitting using gauss functions

    if lambda_ is not None:
        plot_xyz(lambda_, X, Y, Z)
        plot_xyz_nu(lambda_, X, Y, Z)
        plot_xyz_nu_fft(lambda_, X, Y, Z)
        plt.show()
    else:
        print("Failed to load data.")
